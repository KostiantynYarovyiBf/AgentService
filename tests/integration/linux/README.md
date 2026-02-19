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

## What the test does

- runs cleanup first
- creates two namespaces
- starts Python mock control plane
- starts two agents (one per namespace)
- verifies both have `wg0`
- verifies each has at least one peer
- runs cleanup at the end

## Manual env scripts (optional)

```bash
./tests/integration/linux/fixtures/setup_env.sh
./tests/integration/linux/fixtures/cleanup_env.sh
```# Linux Integration Testing

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
sudo -E $(which python3) -m debugpy --listen 5678 --wait-for-client -m pytest -s -vv tests/integration/linux/test_two_peers_init.py::test_two_agents_init_and_exchange_peers

## What the test does

- runs cleanup first
- creates two namespaces
- starts Python mock control plane
- starts two agents (one per namespace)
- verifies both have `wg0`
- verifies each has at least one peer
- runs cleanup at the end

## Manual env scripts (optional)

```bash
sudo ./tests/integration/linux/fixtures/setup_env.sh
sudo ./tests/integration/linux/fixtures/cleanup_env.sh
```
