#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common.sh"

require_cmd ip
require_cmd awk
require_cmd grep
require_cmd mkdir

echo "[setup] Preparing Linux namespaces for integration tests"

"${SCRIPT_DIR}/cleanup_env.sh" --quiet || true

mkdir -p "${STATE_DIR}"
mkdir -p "${ARTIFACTS_DIR}"

run_root ip netns add "${NS_A}"
run_root ip netns add "${NS_B}"

run_root ip link add "${VETH_A_HOST}" type veth peer name "${VETH_A_NS}"
run_root ip link add "${VETH_B_HOST}" type veth peer name "${VETH_B_NS}"

run_root ip link set "${VETH_A_NS}" netns "${NS_A}"
run_root ip link set "${VETH_B_NS}" netns "${NS_B}"

run_root ip addr add "${NS_A_HOST_IP}/${NS_A_CIDR}" dev "${VETH_A_HOST}"
run_root ip link set "${VETH_A_HOST}" up

run_root ip addr add "${NS_B_HOST_IP}/${NS_B_CIDR}" dev "${VETH_B_HOST}"
run_root ip link set "${VETH_B_HOST}" up

run_root ip -n "${NS_A}" link set lo up
run_root ip -n "${NS_A}" addr add "${NS_A_AGENT_IP}/${NS_A_CIDR}" dev "${VETH_A_NS}"
run_root ip -n "${NS_A}" link set "${VETH_A_NS}" up
run_root ip -n "${NS_A}" route add default via "${NS_A_HOST_IP}"

run_root ip -n "${NS_B}" link set lo up
run_root ip -n "${NS_B}" addr add "${NS_B_AGENT_IP}/${NS_B_CIDR}" dev "${VETH_B_NS}"
run_root ip -n "${NS_B}" link set "${VETH_B_NS}" up
run_root ip -n "${NS_B}" route add default via "${NS_B_HOST_IP}"

# Allow forwarding between the two veth pairs (host may have FORWARD DROP from Docker/ufw)
run_root iptables -I FORWARD -i "${VETH_A_HOST}" -o "${VETH_B_HOST}" -j ACCEPT
run_root iptables -I FORWARD -i "${VETH_B_HOST}" -o "${VETH_A_HOST}" -j ACCEPT

write_env_file

echo "[setup] Done"
echo "[setup] Env file: ${ENV_FILE}"
echo "[setup] Namespace A reaches host control-plane at: http://${NS_A_HOST_IP}:${CONTROL_PLANE_PORT}/_mock/openapi"
echo "[setup] Namespace B reaches host control-plane at: http://${NS_B_HOST_IP}:${CONTROL_PLANE_PORT}/_mock/openapi"