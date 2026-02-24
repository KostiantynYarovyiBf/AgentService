from __future__ import annotations

import shlex
import subprocess
import time
from pathlib import Path
from urllib.parse import urlparse

import pytest

from utils import (
    AGENT_BIN,
    CLEANUP_SCRIPT,
    ENV_FILE,
    MOCK_SERVER,
    SETUP_SCRIPT,
    WG_IFACE,
    _has_non_interactive_sudo,
    _parse_env_file,
    _peer_count,
    _root_prefix,
    _run,
    _tail,
    _wait_for_interface,
    _wait_for_peers,
    _get_wg0_ip,
    _wg0_rx_bytes,
    cleanup_processes,
    start_process,
)


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

    try:
        start_process(
            "mock",
            _root_prefix() + [
                "python3", str(MOCK_SERVER),
                "--host", "0.0.0.0",
                "--port", control_plane_port,
                "--ttl", "5",
                "--log-level", "INFO",
            ],
            mock_log,
            processes,
        )
        time.sleep(1)

        start_process(
            "agent-a",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_a,
                str(AGENT_BIN), "--control-plane-url", url_a,
            ],
            agent_a_log,
            processes,
        )
        time.sleep(1)

        start_process(
            "agent-b",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_b,
                str(AGENT_BIN), "--control-plane-url", url_b,
            ],
            agent_b_log,
            processes,
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
        cleanup_processes(processes)


def test_peer_to_peer_icmp_ping(integration_env: dict[str, str]) -> None:
    """Verify that ICMP traffic actually flows through wg0 between the two peers.

    The test:
    1. Starts agents A and B, waits until each has a WireGuard peer configured.
    2. Sends 5 ICMP echo requests from ns-a's wg0 to ns-b's wg0 VPN IP.
    3. Asserts the ping exits with code 0 (all packets received).
    4. Asserts ns-b's wg0 RX byte counter increased, confirming the ICMP
       packets were decrypted and received on the WireGuard interface
       (not leaked out a plain veth).
    """
    if not AGENT_BIN.exists():
        pytest.fail(f"Missing AgentService binary: {AGENT_BIN}")
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

    mock_log = state_dir / "pytest_mock_ping.log"
    agent_a_log = state_dir / "pytest_agent_a_ping.log"
    agent_b_log = state_dir / "pytest_agent_b_ping.log"

    processes: list[tuple[str, subprocess.Popen[str]]] = []

    try:
        start_process(
            "mock",
            _root_prefix() + [
                "python3", str(MOCK_SERVER),
                "--host", "0.0.0.0",
                "--port", control_plane_port,
                "--ttl", "30",
                "--log-level", "INFO",
            ],
            mock_log,
            processes,
        )
        time.sleep(1)

        start_process(
            "agent-a",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_a,
                str(AGENT_BIN), "--control-plane-url", url_a,
            ],
            agent_a_log,
            processes,
        )
        time.sleep(1)

        start_process(
            "agent-b",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_b,
                str(AGENT_BIN), "--control-plane-url", url_b,
            ],
            agent_b_log,
            processes,
        )

        assert _wait_for_interface(
            namespace_a, timeout_sec=25), "wg0 not created in namespace A"
        assert _wait_for_interface(
            namespace_b, timeout_sec=25), "wg0 not created in namespace B"
        assert _wait_for_peers(namespace_a, min_peers=1,
                               timeout_sec=45), "namespace A has no peers"
        assert _wait_for_peers(namespace_b, min_peers=1,
                               timeout_sec=45), "namespace B has no peers"

        # Discover each peer's wg0 VPN IP
        vpn_ip_a = _get_wg0_ip(namespace_a)
        vpn_ip_b = _get_wg0_ip(namespace_b)
        assert vpn_ip_a, "Could not determine wg0 IP for namespace A"
        assert vpn_ip_b, "Could not determine wg0 IP for namespace B"

        # Baseline RX counters on both wg0 interfaces before sending ICMP
        rx_before_b = _wg0_rx_bytes(namespace_b)
        rx_before_a = _wg0_rx_bytes(namespace_a)

        # Ping A → B through WireGuard
        ping_ab = _run(
            _root_prefix() + [
                "ip", "netns", "exec", namespace_a,
                "ping", "-c", "5", "-W", "2", "-I", WG_IFACE, vpn_ip_b,
            ],
            check=False,
        )
        assert ping_ab.returncode == 0, (
            f"Ping A→B failed (vpn_ip_b={vpn_ip_b}):\n{ping_ab.stdout}\n{ping_ab.stderr}"
        )

        # Ping B → A through WireGuard
        ping_ba = _run(
            _root_prefix() + [
                "ip", "netns", "exec", namespace_b,
                "ping", "-c", "5", "-W", "2", "-I", WG_IFACE, vpn_ip_a,
            ],
            check=False,
        )
        assert ping_ba.returncode == 0, (
            f"Ping B→A failed (vpn_ip_a={vpn_ip_a}):\n{ping_ba.stdout}\n{ping_ba.stderr}"
        )

        # Verify ICMP actually traversed wg0 (RX bytes must have increased)
        rx_after_b = _wg0_rx_bytes(namespace_b)
        rx_after_a = _wg0_rx_bytes(namespace_a)
        assert rx_after_b > rx_before_b, (
            f"wg0 in namespace B received no bytes after ping "
            f"(before={rx_before_b}, after={rx_after_b}) — traffic did not go through WireGuard"
        )
        assert rx_after_a > rx_before_a, (
            f"wg0 in namespace A received no bytes after ping "
            f"(before={rx_before_a}, after={rx_after_a}) — traffic did not go through WireGuard"
        )

    except AssertionError as exc:
        debug = (
            f"\nAssertion: {exc}\n"
            f"\nmock tail:\n{_tail(mock_log)}\n"
            f"\nagent A tail:\n{_tail(agent_a_log)}\n"
            f"\nagent B tail:\n{_tail(agent_b_log)}\n"
        )
        pytest.fail(debug)
    finally:
        cleanup_processes(processes)


def test_peer_update_propagation(integration_env: dict[str, str]) -> None:
    """Agent A is started first (solo, 0 peers), then B registers; A must acquire B as a peer.

    This verifies dynamic peer propagation: A's heartbeat must fetch the updated
    peer list from the control plane after B registers and configure wg0 accordingly.
    """
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

    mock_log = state_dir / "pytest_mock_prop.log"
    agent_a_log = state_dir / "pytest_agent_a_prop.log"
    agent_b_log = state_dir / "pytest_agent_b_prop.log"

    processes: list[tuple[str, subprocess.Popen[str]]] = []

    try:
        start_process(
            "mock",
            _root_prefix() + [
                "python3", str(MOCK_SERVER),
                "--host", "0.0.0.0",
                "--port", control_plane_port,
                "--ttl", "10",
                "--log-level", "INFO",
            ],
            mock_log,
            processes,
        )
        time.sleep(1)

        start_process(
            "agent-a",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_a,
                str(AGENT_BIN), "--control-plane-url", url_a,
            ],
            agent_a_log,
            processes,
        )

        assert _wait_for_interface(
            namespace_a, timeout_sec=25), "wg0 not created in namespace A"

        # A is the only registered agent — it must start with zero peers.
        assert _peer_count(namespace_a) == 0, (
            f"Expected 0 peers in namespace A before B registers, "
            f"got {_peer_count(namespace_a)}"
        )

        start_process(
            "agent-b",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_b,
                str(AGENT_BIN), "--control-plane-url", url_b,
            ],
            agent_b_log,
            processes,
        )

        assert _wait_for_interface(
            namespace_b, timeout_sec=25), "wg0 not created in namespace B"

        # A must pick up B on the next heartbeat cycle (TTL=10 → interval ≤ 9 s).
        assert _wait_for_peers(namespace_a, min_peers=1, timeout_sec=30), (
            "namespace A never acquired B as a peer after B registered"
        )
        assert _wait_for_peers(namespace_b, min_peers=1, timeout_sec=30), (
            "namespace B never acquired A as a peer"
        )

    except AssertionError as exc:
        pytest.fail(
            f"\nAssertion: {exc}\n"
            f"\nmock tail:\n{_tail(mock_log)}\n"
            f"\nagent A tail:\n{_tail(agent_a_log)}\n"
            f"\nagent B tail:\n{_tail(agent_b_log)}\n"
        )
    finally:
        cleanup_processes(processes)


def test_no_change_heartbeat_304(integration_env: dict[str, str]) -> None:
    """Agents that receive HTTP 304 (topology unchanged) must keep the tunnel healthy.

    With TTL=6 the heartbeat fires every 5 s.  After ≥2 cycles without any
    topology change the mock returns 304.  We assert both agents are still
    running and each wg0 still has its peer configured.
    """
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

    # TTL=6 → heartbeat interval = max(1, 6-1) = 5 s.
    heartbeat_interval_sec = 5
    mock_log = state_dir / "pytest_mock_304.log"
    agent_a_log = state_dir / "pytest_agent_a_304.log"
    agent_b_log = state_dir / "pytest_agent_b_304.log"

    processes: list[tuple[str, subprocess.Popen[str]]] = []

    try:
        start_process(
            "mock",
            _root_prefix() + [
                "python3", str(MOCK_SERVER),
                "--host", "0.0.0.0",
                "--port", control_plane_port,
                "--ttl", "6",
                "--log-level", "INFO",
            ],
            mock_log,
            processes,
        )
        time.sleep(1)

        for ns, url, log in [
            (namespace_a, url_a, agent_a_log),
            (namespace_b, url_b, agent_b_log),
        ]:
            start_process(
                f"agent-{ns}",
                _root_prefix() + [
                    "ip", "netns", "exec", ns,
                    str(AGENT_BIN), "--control-plane-url", url,
                ],
                log,
                processes,
            )
            time.sleep(1)

        assert _wait_for_interface(
            namespace_a, timeout_sec=25), "wg0 not created in namespace A"
        assert _wait_for_interface(
            namespace_b, timeout_sec=25), "wg0 not created in namespace B"
        assert _wait_for_peers(namespace_a, min_peers=1,
                               timeout_sec=45), "namespace A has no peers"
        assert _wait_for_peers(namespace_b, min_peers=1,
                               timeout_sec=45), "namespace B has no peers"

        # Let at least 2 full heartbeat cycles elapse so 304 responses are issued.
        time.sleep(heartbeat_interval_sec * 2 + 2)

        # Both agents must still be alive.
        for name, proc in processes:
            assert proc.poll() is None, (
                f"Process '{name}' exited unexpectedly (rc={proc.returncode}) "
                f"after receiving 304 heartbeat responses"
            )

        # Tunnel must still be healthy — wg0 exists and peers are still configured.
        assert _wait_for_interface(
            namespace_a, timeout_sec=5), "wg0 disappeared in namespace A after 304 heartbeats"
        assert _wait_for_interface(
            namespace_b, timeout_sec=5), "wg0 disappeared in namespace B after 304 heartbeats"
        assert _peer_count(
            namespace_a) >= 1, "namespace A lost its peer after 304 heartbeats"
        assert _peer_count(
            namespace_b) >= 1, "namespace B lost its peer after 304 heartbeats"

    except AssertionError as exc:
        pytest.fail(
            f"\nAssertion: {exc}\n"
            f"\nmock tail:\n{_tail(mock_log)}\n"
            f"\nagent A tail:\n{_tail(agent_a_log)}\n"
            f"\nagent B tail:\n{_tail(agent_b_log)}\n"
        )
    finally:
        cleanup_processes(processes)


def test_ttl_expiry_failsafe(integration_env: dict[str, str]) -> None:
    """When the control plane becomes unreachable the agent must not crash.

    Expected behavior (current implementation): agent keeps retrying the
    heartbeat, logs errors, and does NOT terminate abnormally.
    If a hard fail-safe (tunnel teardown) is implemented in the future this
    test should be updated to assert that wg0 is removed instead.
    """
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

    ttl_sec = 3
    mock_log = state_dir / "pytest_mock_ttl.log"
    agent_a_log = state_dir / "pytest_agent_a_ttl.log"
    agent_b_log = state_dir / "pytest_agent_b_ttl.log"

    mock_processes: list[tuple[str, subprocess.Popen[str]]] = []
    agent_processes: list[tuple[str, subprocess.Popen[str]]] = []

    try:
        start_process(
            "mock",
            _root_prefix() + [
                "python3", str(MOCK_SERVER),
                "--host", "0.0.0.0",
                "--port", control_plane_port,
                "--ttl", str(ttl_sec),
                "--log-level", "INFO",
            ],
            mock_log,
            mock_processes,
        )
        time.sleep(1)

        start_process(
            "agent-a",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_a,
                str(AGENT_BIN), "--control-plane-url", url_a,
            ],
            agent_a_log,
            agent_processes,
        )
        time.sleep(1)
        start_process(
            "agent-b",
            _root_prefix() + [
                "ip", "netns", "exec", namespace_b,
                str(AGENT_BIN), "--control-plane-url", url_b,
            ],
            agent_b_log,
            agent_processes,
        )

        assert _wait_for_interface(
            namespace_a, timeout_sec=25), "wg0 not created in namespace A"
        assert _wait_for_interface(
            namespace_b, timeout_sec=25), "wg0 not created in namespace B"
        assert _wait_for_peers(
            namespace_a, min_peers=1, timeout_sec=30), "namespace A has no peers before mock shutdown"
        assert _wait_for_peers(
            namespace_b, min_peers=1, timeout_sec=30), "namespace B has no peers before mock shutdown"

        # Bring down the control plane.
        cleanup_processes(mock_processes)

        # Wait well past TTL so eviction and multiple failed heartbeat retries occur.
        time.sleep(ttl_sec * 3)

        # Agent must not have crashed (SIGSEGV / unhandled exception).
        # Acceptable outcomes: still running (rc=None) or clean exit (rc=0).
        for name, proc in agent_processes:
            rc = proc.poll()
            assert rc is None or rc == 0, (
                f"Agent '{name}' terminated abnormally after control plane shutdown "
                f"(rc={rc}). Expected graceful retry or clean exit."
            )

    except AssertionError as exc:
        pytest.fail(
            f"\nAssertion: {exc}\n"
            f"\nmock tail:\n{_tail(mock_log)}\n"
            f"\nagent A tail:\n{_tail(agent_a_log)}\n"
            f"\nagent B tail:\n{_tail(agent_b_log)}\n"
        )
    finally:
        cleanup_processes(agent_processes)
        cleanup_processes(mock_processes)


def test_idempotent_setup_cleanup() -> None:
    """Running setup→cleanup→setup must succeed with no leaked namespaces or interfaces.

    This validates CI reliability: a failed previous run must not prevent the
    next run from starting cleanly.  No agents or mock server are involved.
    """
    if not _has_non_interactive_sudo():
        pytest.skip(
            "Integration test requires root or passwordless sudo (sudo -n)")

    for script in (SETUP_SCRIPT, CLEANUP_SCRIPT):
        if not script.exists():
            pytest.fail(f"Missing required script: {script}")

    def _namespace_exists(ns: str) -> bool:
        result = _run(_root_prefix() + ["ip", "netns", "list"], check=False)
        return any(line.split()[0] == ns for line in result.stdout.splitlines() if line.strip())

    # First cycle: setup → cleanup
    _run([str(SETUP_SCRIPT)])
    assert ENV_FILE.exists(
    ), f"Env file not written after first setup: {ENV_FILE}"
    env = _parse_env_file(ENV_FILE)
    ns_a, ns_b = env["NS_A"], env["NS_B"]
    assert _namespace_exists(
        ns_a), f"Namespace {ns_a} missing after first setup"
    assert _namespace_exists(
        ns_b), f"Namespace {ns_b} missing after first setup"

    _run([str(CLEANUP_SCRIPT), "--quiet"])
    assert not _namespace_exists(
        ns_a), f"Namespace {ns_a} still present after cleanup"
    assert not _namespace_exists(
        ns_b), f"Namespace {ns_b} still present after cleanup"

    # Second cycle: setup must succeed cleanly even after cleanup.
    _run([str(SETUP_SCRIPT)])
    assert ENV_FILE.exists(
    ), f"Env file not written after second setup: {ENV_FILE}"
    assert _namespace_exists(
        ns_a), f"Namespace {ns_a} missing after second setup"
    assert _namespace_exists(
        ns_b), f"Namespace {ns_b} missing after second setup"

    # Final teardown.
    _run([str(CLEANUP_SCRIPT), "--quiet"])
    assert not _namespace_exists(
        ns_a), f"Namespace {ns_a} leaked after final cleanup"
    assert not _namespace_exists(
        ns_b), f"Namespace {ns_b} leaked after final cleanup"
