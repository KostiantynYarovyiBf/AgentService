from __future__ import annotations

import os
import subprocess
import time
from pathlib import Path

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
            _root_prefix() + ["ip", "-n", namespace, "link", "show", WG_IFACE],
            check=False,
        )
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


def _get_wg0_ip(namespace: str) -> str:
    """Return the IP address (without prefix) assigned to wg0 in *namespace*."""
    result = _run(
        _root_prefix() + ["ip", "-n", namespace, "addr", "show", WG_IFACE],
        check=False,
    )
    for line in result.stdout.splitlines():
        line = line.strip()
        if line.startswith("inet "):
            return line.split()[1].split("/")[0]
    return ""


def _wg0_rx_bytes(namespace: str) -> int:
    """Return total received bytes reported by 'wg show wg0 transfer' for *namespace*."""
    result = _run(
        _root_prefix() + ["ip", "netns", "exec", namespace,
                          "wg", "show", WG_IFACE, "transfer"],
        check=False,
    )
    total = 0
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2:
            try:
                total += int(parts[1])  # column 1 = received bytes
            except ValueError:
                pass
    return total


def start_process(
    name: str,
    cmd: list[str],
    log_path: Path,
    processes: list[tuple[str, subprocess.Popen[str]]],
) -> None:
    """Spawn *cmd*, redirect stdout+stderr to *log_path*, and register in *processes*."""
    handle = log_path.open("w", encoding="utf-8")
    proc = subprocess.Popen(
        cmd, stdout=handle, stderr=subprocess.STDOUT, text=True)
    processes.append((name, proc))


def cleanup_processes(processes: list[tuple[str, subprocess.Popen[str]]]) -> None:
    """Terminate all processes gracefully, killing any that do not stop in time."""
    for _name, proc in processes:
        if proc.poll() is None:
            proc.terminate()
    for _name, proc in processes:
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
