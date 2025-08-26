## Network Features

### ACL
- interface is fixed to wg0; the plugin applies all chains only when iifname/oifname == wg0.
- chain semantics:
    input = to this host via wg0 (post-decrypt inner traffic)
    forward = routed through this host via wg0
    output = from this host out via wg0

- priority: lower runs first; omit if you want the plugin to auto-order.
- ct_state allowed: ["new","established","related","invalid"].
- On wg0 (L3), MAC filtering is not meaningful—use IP/port/proto.
- If you ever need to match pre-NAT identities, add fields like "ct_original": { "saddr": "...", "daddr": "...", "sport": 1234, "dport": 80 }.

**Config example:**
```
{
  "version": 42,                       // atomic config version
  "specs": [
    {
      "plugin_id": "acl",
      "execType": "job",               // "service" | "job"
      "pkgLing": "link/to/pkg",        // Link for download a packed version
      "pkgVersion": "1.0.0",           // requested package version (optional)
      "enabled": true,                 // service: should run; job: should (re)apply when config changes
      "keepInstalled": true,           // if later omitted from specs, keep package on disk
      "config": {}
    },
    {
      "plugin_id": "dpi_logger",
      "execType": "service",
      "pkgVersion": "2.0.0",
      "enabled": false,                // stop service but keep installed
      "keepInstalled": true,
      "config": {}
    }
  ],
  "policy": { "uninstallRemoved": false }  // if a plugin disappears from specs
}


```

# Mirroring
