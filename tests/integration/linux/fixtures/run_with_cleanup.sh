#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if [[ "$#" -eq 0 ]]; then
  echo "Usage: $0 <command> [args...]" >&2
  exit 1
fi

cleanup() {
  "${SCRIPT_DIR}/cleanup_env.sh" --quiet || true
}

trap cleanup EXIT

"${SCRIPT_DIR}/setup_env.sh"
"$@"