# PluginManager (VPN Agent) — Job-Plugin Management (Agent-Side)

## 1. Role & Responsibilities

PluginManager is the single entry point inside the VPN Agent for managing external plugins that apply network state and exit.

It guarantees:
- Safe discovery (paths, ownership, manifest).
- Clean lifecycle for each operation: handshake → install / apply_cfg / uninstall / validate_only.
- Strict idempotency and atomicity (via generation).
- One in-flight operation per plugin name (serialized calls).
- Two transport options for jobs:
    - JSON over stdio (preferred).
    - Mailbox Spool (filesystem, optional fallback).
- Uniform error mapping, timeouts, logging, and sandboxing.

---

## 2. Discovery & Security (Agent-Side)

**Locations**
- Executable: `/usr/lib/vpn-agent/plugins/<plugin>/<plugin>`
- Optional manifest: `/usr/lib/vpn-agent/plugins/<plugin>/manifest.json`

**Pre-flight checks (must pass or abort):**
- Absolute path under the directory above; no symlinks in path.
- Owner: root (or service user agent); perms 0755 (exe), 0644 (manifest).
- Optional signature/hash verification (deployment policy).

**Compatibility gates:**
- Manifest or handshake must declare abi (major) and capabilities (e.g., ACL, MIRROR, QOS, WG_CONFIG, XDP_LOAD).
- PluginManager rejects incompatible abi or missing required capabilities before launch.

---

## 3. Operations & Semantics

Every request carries:
- `op`: "handshake" | "install" | "apply_cfg" | "uninstall" | "validate_only"
- `generation`: monotonic integer; idempotency key per plugin.
- `capabilities_required`: subset the agent expects for this call.
- `cfg`: plugin-specific config (JSON object).
- `msg_id`: UUID for correlation.

**Rules:**
- Idempotency: same (plugin, generation) must be safe to replay.
- Atomicity: apply_cfg either fully applies or rolls back; no partial state left behind.
- Ownership: plugin creates only namespaced kernel objects (e.g., vpnA_<plugin>_<hash>); never clobber foreign state.

---

## 4. Transport A — JSON over stdio (Preferred)

**Lifecycle per operation:**
- PluginManager spawns the plugin (direct exec or systemd-run --wait).
- Writes one UTF-8 JSON request to stdin.
- Reads one UTF-8 JSON response from stdout and process exit code.
- Enforces size limits and deadline.

**Request (stdin) — minimal shape:**
```json
{
    "abi": 1,
    "op": "apply_cfg",
    "msg_id": "uuid",
    "generation": 42,
    "capabilities_required": ["ACL"],
    "cfg": { /* plugin-specific */ },
    "context": { /* optional host info */ }
}
```

**Response (stdout):**
```json
{
    "ok": true,
    "msg_id": "uuid",
    "abi": 1,
    "capabilities": ["ACL","IPV4","IPV6"],
    "applied_generation": 42,
    "counters": { "rules_installed": 18 },
    "err": null,
    "notes": []
}
```

**Exit codes:**
- 0 iff ok:true; otherwise non-zero (mapped to error class below).

See [diagrams/SequenceDiagram.puml](../diagrams/SequenceDiagram.puml)




## 5. Error Classes (Agent-Side Contract)

Plugins report one of:

| Code             | Meaning                         | Typical Action             |
| ---------------- | ------------------------------- | -------------------------- |
| `E_BADCFG`       | Invalid/unsupported config      | Don’t retry; surface error |
| `E_INCOMPATIBLE` | ABI/capability mismatch         | Block until upgraded       |
| `E_PERM`         | Missing privileges/caps         | Adjust unit caps; retry    |
| `E_TRANSIENT`    | Temporary system state          | Backoff & retry            |
| `E_NOTFOUND`     | Interface/object missing        | Recreate deps; retry       |
| `E_CONFLICT`     | Would clobber foreign resources | Operator action required   |
| `E_INTERNAL`     | Unexpected plugin error         | Short backoff; alert       |
| `E_TIMEOUT`      | Exceeded deadline               | Increase timeout or split  |
| `E_TOOLING`      | Helper/kernel feature missing   | Install prereqs; retry     |


**PluginManager behavior:**
- Correlate by msg_id.
- Record applied_generation on success.
- Centralize logs, counters, last error per plugin.
- Apply exponential backoff for E_TRANSIENT; do not auto-retry E_BADCFG.


## 6. Orchestration & Systemd (Agent-Side)

Launch directly or via transient units for sandboxing:
- CapabilityBoundingSet=CAP_NET_ADMIN CAP_NET_RAW (only if needed)
- NoNewPrivileges=yes, PrivateTmp=yes, ProtectSystem=strict, ProtectHome=yes
- RestrictAddressFamilies=AF_INET AF_INET6 AF_UNIX
- Optional SystemCallFilter= allowlist
- Resource ceilings: CPUQuota=, MemoryMax=

PluginManager always:
- Sets a deadline per job.
- Captures stderr as structured logs (JSON lines).
- Normalizes exit codes to error classes.



## 7. PluginManager External API (inside Agent)

- `install(plugin, generation, cfg, capabilities_required)`. Validates, runs install, updates state on success.
- `apply_cfg(plugin, generation, cfg, capabilities_required)`. Runs apply_cfg; must be called with monotonically increasing generation.
- `uninstall(plugin)`. Cleans namespaced resources; safe to call even if partially present.
- `validate_only(plugin, cfg, capabilities_required)`. Parser/semantic checks only; no mutation.
- `status(plugin)`. Returns last applied_generation, last_ok/err, counters, manifest/handshake info.

See [diagrams/FlowDiagram.puml](../diagrams/FlowDiagram.puml)


## 8. Observability

- Central logging: stdout reserved for single response (stdio mode); stderr of plugin is captured, tagged, and forwarded by the Agent.
- Counters per plugin (maintained by PluginManager): invocations_total, success_total, errors_total{code}, last_duration_ms, last_applied_generation.
- Auditable trail: store minimal op envelope (plugin, op, generation, result, timestamp), not full configs (unless policy requires).


## 9. Defaults & Limits (Agent-Side)

- Max request/response size: 1 MiB.
- Default job deadline: 5s (configurable per plugin/op).
- One in-flight job per plugin; queued requests coalesce by latest generation.
- Path allowlist: only `/usr/lib/vpn-agent/plugins/<plugin>/<plugin>`.
