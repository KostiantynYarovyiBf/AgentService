from __future__ import annotations

import os
import shlex
import subprocess
import time
from collections.abc import Generator
from pathlib import Path
from urllib.parse import urlparse

import pytest

REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPT_DIR = Path(__file__).resolve().parent
FIXTURES_DIR = SCRIPT_DIR / "fixtures"
ARTIFACTS_DIR = SCRIPT_DIR / "artifacts"
SETUP_SCRIPT = FIXTURES_DIR / "setup_env.sh"
CLEANUP_SCRIPT = FIXTURES_DIR / "cleanup_env.sh"
ENV_FILE = ARTIFACTS_DIR / ".env.generated"
AGENT_BIN = REPO_ROOT / "build" / "AgentService"
MOCK_SERVER = REPO_ROOT / "control_plane_mock" / "mock_server.py"
WG_IFACE = "wg0"


def _has_non_interactive_sudo() -> bool:
    if os.geteuid() == 0:
        return True
    check = subprocess.run(["sudo", "-n", "true"],
                           capture_output=True, text=True)
    return check.returncode == 0


def _root_prefix() -> list[str]:
    return [] if os.geteuid() == 0 else ["sudo", "-n"]


def _run(command: list[str], check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, check=check, capture_output=True, text=True)


def _parse_env_file(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for raw_line in path.read_text().splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        data[key.strip()] = value.strip()
    return data


def _tail(path: Path, lines: int = 80) -> str:
    if not path.exists():
        return f"<missing log: {path}>"
    content = path.read_text(errors="ignore").splitlines()
    return "\n".join(content[-lines:])


def _wait_for_interface(namespace: str, timeout_sec: int) -> bool:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        result = _run(
            _root_prefix() + ["ip", "-n", namespace, "link", "show", WG_IFACE], check=False)
        if result.returncode == 0:
            return True
        time.sleep(1)
    return False


def _peer_count(namespace: str) -> int:
    result = _run(
        _root_prefix() + ["ip", "netns", "exec", namespace,
                          "wg", "show", WG_IFACE, "peers"],
        check=False,
    )
    if result.returncode != 0:
        return 0
    return len([line for line in result.stdout.splitlines() if line.strip()])


def _wait_for_peers(namespace: str, min_peers: int, timeout_sec: int) -> bool:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if _peer_count(namespace) >= min_peers:
            return True
        time.sleep(1)
    return False


@pytest.fixture(name="integration_env")
def fixture_integration_env() -> Generator[dict[str, str], None, None]:
    if not _has_non_interactive_sudo():
        pytest.skip(
            "Integration test requires root or passwordless sudo (sudo -n)")

    for cmd in (SETUP_SCRIPT, CLEANUP_SCRIPT):
        if not cmd.exists():
            pytest.fail(f"Missing required script: {cmd}")

    _run([str(CLEANUP_SCRIPT), "--quiet"], check=False)
    _run([str(SETUP_SCRIPT)])

    if not ENV_FILE.exists():
        pytest.fail(f"Expected env file not generated: {ENV_FILE}")

    env_data = _parse_env_file(ENV_FILE)
    try:
        yield env_data
    finally:
        _run([str(CLEANUP_SCRIPT), "--quiet"], check=False)


def test_two_agents_init_and_exchange_peers(integration_env: dict[str, str]) -> None:
    if not AGENT_BIN.exists():
        pytest.fail(
            f"Missing AgentService binary: {AGENT_BIN}. Build with: cmake --build build")
    if not MOCK_SERVER.exists():
        pytest.fail(f"Missing mock server: {MOCK_SERVER}")

    namespace_a = integration_env["NS_A"]
    namespace_b = integration_env["NS_B"]
    url_a = integration_env["CONTROL_PLANE_URL_A"]
    url_b = integration_env["CONTROL_PLANE_URL_B"]
    state_dir = Path(integration_env["STATE_DIR"])
    state_dir.mkdir(parents=True, exist_ok=True)

    parsed = urlparse(url_a)
    control_plane_port = str(parsed.port or 4000)

    mock_log = state_dir / "pytest_mock.log"
    agent_a_log = state_dir / "pytest_agent_a.log"
    agent_b_log = state_dir / "pytest_agent_b.log"

    processes: list[tuple[str, subprocess.Popen[str]]] = []

    def _start(name: str, cmd: list[str], log_path: Path) -> None:
        handle = log_path.open("w", encoding="utf-8")
        proc = subprocess.Popen(
            cmd, stdout=handle, stderr=subprocess.STDOUT, text=True)
        processes.append((name, proc))

    def _cleanup_processes() -> None:
        for _name, proc in processes:
            if proc.poll() is None:
                proc.terminate()
        for _name, proc in processes:
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()

    try:
        _start(
            "mock",
            _root_prefix()
            + [
                "python3",
                str(MOCK_SERVER),
                "--host",
                "0.0.0.0",
                "--port",
                control_plane_port,
                "--ttl",
                "5",
                "--log-level",
                "INFO",
            ],
            mock_log,
        )
        time.sleep(1)

        _start(
            "agent-a",
            _root_prefix()
            + [
                "ip",
                "netns",
                "exec",
                namespace_a,
                str(AGENT_BIN),
                "--control-plane-url",
                url_a,
            ],
            agent_a_log,
        )

        time.sleep(1)

        _start(
            "agent-b",
            _root_prefix()
            + [
                "ip",
                "netns",
                "exec",
                namespace_b,
                str(AGENT_BIN),
                "--control-plane-url",
                url_b,
            ],
            agent_b_log,
        )

        assert _wait_for_interface(
            namespace_a, timeout_sec=25), "wg0 was not created in namespace A"
        assert _wait_for_interface(
            namespace_b, timeout_sec=25), "wg0 was not created in namespace B"

        assert _wait_for_peers(namespace_a, min_peers=1,
                               timeout_sec=45), "namespace A has no peers"
        assert _wait_for_peers(namespace_b, min_peers=1,
                               timeout_sec=45), "namespace B has no peers"

    except AssertionError as exc:
        debug = (
            f"\nAssertion: {exc}\n"
            f"\nmock tail:\n{_tail(mock_log)}\n"
            f"\nagent A tail:\n{_tail(agent_a_log)}\n"
            f"\nagent B tail:\n{_tail(agent_b_log)}\n"
            f"\nCommands:\n"
            f"A: {shlex.join(_root_prefix() + ['ip', 'netns', 'exec', namespace_a, str(AGENT_BIN), '--control-plane-url', url_a])}\n"
            f"B: {shlex.join(_root_prefix() + ['ip', 'netns', 'exec', namespace_b, str(AGENT_BIN), '--control-plane-url', url_b])}\n"
        )
        pytest.fail(debug)
    finally:
        _cleanup_processes()
