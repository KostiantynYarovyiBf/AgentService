# Linux Integration Testing

This folder contains Linux integration test infrastructure:

- namespace lifecycle scripts (`setup_env.sh`, `cleanup_env.sh`)
- pytest scenario tests (starting with `test_two_peers_init.py`)
- generated artifacts in `tests/integration/linux/artifacts/` (including `.env.generated`)

## Prerequisites

- built agent binary at `build/AgentService`
- `python3`, `pytest`, `ip`, `wg`
- root or passwordless sudo (`sudo -n`)

## Run integration tests

# 1 test
```bash
sudo -E $(which python3) -m pytest -s -vv tests/integration/linux/test_two_peers_init.py::test_two_agents_init_and_exchange_peers
```
# Whole file
```bash
sudo -E $(which python3) -m pytest -s -vv tests/integration/linux/test_two_peers_init.py
```

## Debug tests
```bash
sudo -E $(which python3) -m debugpy --listen 5678 --wait-for-client -m pytest -s -vv tests/integration/linux/test_two_peers_init.py::test_two_agents_init_and_exchange_peers
```

## Test topology

```
  HOST
  ├── Mock Control Plane (127.0.0.1:4000)
  │     ▲ POST /register            ▲ POST /register
  │     │ GET  /ping                │ GET  /ping
  │     │                           │
  ├── veth-vpn-a-host               ├── veth-vpn-b-host
  │   (172.31.201.1/24)             │   (172.31.202.1/24)
  │     │                           │
  │  ┌──┴──────────────────┐     ┌──┴──────────────────┐
  │  │      vpn-ns-a       │     │      vpn-ns-b       │
  │  │  veth  172.31.201.x │     │  veth  172.31.202.x │
  │  │  wg0   10.200.0.2/32│     │  wg0   10.200.0.3/32│
  │  │  AgentService       │     │  AgentService       │
  │  └─────────┬───────────┘     └────────────┬────────┘
  │            │                              │
  │            └─────── WireGuard ────────────┘
  │                  (10.200.0.2 ↔ 10.200.0.3)
```

## What the test does

- runs cleanup first
- creates two namespaces (`vpn-ns-a`, `vpn-ns-b`) each with a veth pair to host
- starts Python mock control plane on `127.0.0.1:4000`
- starts two agents (one per namespace), each registers with the mock CP
- verifies both have `wg0`
- verifies each has at least one peer
- runs cleanup at the end

## Manual env scripts (optional)

```bash
./tests/integration/linux/fixtures/setup_env.sh
./tests/integration/linux/fixtures/cleanup_env.sh
```
