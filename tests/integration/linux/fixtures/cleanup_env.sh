#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

QUIET=0
if [[ "${1:-}" == "--quiet" ]]; then
  QUIET=1
fi

log() {
  if [[ "${QUIET}" -eq 0 ]]; then
    echo "$1"
  fi
}

run_root_cmd() {
  if run_root "$@"; then
    return 0
  fi
  return 1
}

log "[cleanup] Cleaning integration test environment"

for pid_file in "${STATE_DIR}"/*.pid; do
  [[ -e "${pid_file}" ]] || continue
  pid="$(cat "${pid_file}" 2>/dev/null || true)"
  if [[ -n "${pid}" ]]; then
    run_root_cmd kill "${pid}" >/dev/null 2>&1 || true
  fi
done

if namespace_exists "${NS_A}"; then
  run_root_cmd ip netns pids "${NS_A}" | xargs -r run_root kill >/dev/null 2>&1 || true
fi
if namespace_exists "${NS_B}"; then
  run_root_cmd ip netns pids "${NS_B}" | xargs -r run_root kill >/dev/null 2>&1 || true
fi

if link_exists "${VETH_A_HOST}"; then
  run_root_cmd ip link del "${VETH_A_HOST}" >/dev/null 2>&1 || true
fi
if link_exists "${VETH_B_HOST}"; then
  run_root_cmd ip link del "${VETH_B_HOST}" >/dev/null 2>&1 || true
fi

if namespace_exists "${NS_A}"; then
  run_root_cmd ip netns del "${NS_A}" >/dev/null 2>&1 || true
fi
if namespace_exists "${NS_B}"; then
  run_root_cmd ip netns del "${NS_B}" >/dev/null 2>&1 || true
fi

run_root_cmd rm -rf "${STATE_DIR}" >/dev/null 2>&1 || true
rm -f "${ENV_FILE}" || true

log "[cleanup] Done"