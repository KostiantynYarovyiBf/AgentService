# EdgeBox VPN — Architecture Overview

## Overview of VPN Architecture
```
+-----------------------------------------------------------+
|                       Control Plane                       |
+-----------------------------------------------------------+
| Device Registry   | Auth Service   | Module Manager       |
| State Manager     | API Gateway    | Database             |
+-----------------------------------------------------------+
                |            |             | 
                |            |             |
        --------+------------+-------------+---------------------
        |                  |               |                    |
+---------------+  +---------------+  +---------------+  +---------------+
|   VPN Agent A |  |   VPN Agent B |  |   VPN Agent C |  |   VPN Agent D |
|   (Modules)   |  |   (Modules)   |  |   (Modules)   |  |   (Modules)   |
+---------------+  +---------------+  +---------------+  +---------------+
        |                  |               |                    |
        ---------------------------------------------------------
                      Secure WireGuard Mesh
```

### 1. Control Plane

The Control Plane is the central management service that coordinates all VPN agents.  
It is responsible for:

- Device registration & authentication
- Peer discovery and configuration
- Module assignment & OTA updates
- State tracking and analytics

**Main Components:**
- **Device Registry:** Stores peer public keys, IPs, and metadata
- **Auth Service:** Manages authentication and tokens
- **Module Manager:** Assigns active modules to agents based on configuration or subscriptions
- **State Manager:** Tracks agent status and peer connectivity
- **API Gateway:** Handles communication between agents and Control Plane

---

### 2. VPN Agents

VPN Agents run on peer devices (Linux VMs, servers, workstations).  
Each agent:

- Registers with the Control Plane (`/register`)
- Receives peer list & ACL updates
- Maintains WireGuard connections
- Runs a pipeline of Pre‑VPN, In‑VPN, and Post‑VPN modules

**Agent Components:**
- **Registration Service:** Connects to Control Plane, sends heartbeats, applies config changes
- **Config & Module Manager:** Loads/unloads modules dynamically
- **Pipeline:** Processes packets:
  - **Pre‑VPN Modules:** Operate on plaintext packets before encryption
  - **In‑VPN Modules:** Operate on encrypted packets (inline)
  - **Post‑VPN Modules:** Operate on plaintext after decryption
- **WireGuard Interface (TUN):** Handles encrypted tunnel traffic

---

### 3. Communication Flow

**Agent Registration**
1. Agent starts and registers with Control Plane (public key, device info)
2. Control Plane stores the peer in Device Registry
3. Control Plane returns the full peer list + active module list
4. Agent configures WireGuard and pipeline modules accordingly

**Peer Update**
- A new peer joins → Control Plane updates Device Registry
- Control Plane pushes updated peer list to all existing agents
- Existing agents update ACLs and AllowedIPs dynamically (no restart required)

---

### 4. Topology

- **Phase 1 (MVP):** Star topology  
  All agents connect to Control Plane host as central hub.  
  Traffic between peers is routed via Control Plane’s WireGuard instance.

- **Phase 2:** Full mesh topology  
  Control Plane coordinates direct peer‑to‑peer connections.  
  Agents exchange AllowedIPs to establish direct encrypted tunnels.

---

### 5. Deployment

- Control Plane runs on server (Go)
- VPN Agents run as system services on Linux (C++), later on Windows/macOS with OS‑specific pipeline
- Docker can be used for testing multiple agents in lab setups

---
# EdgeBox VPN Agent — Architecture Overview

```
+-----------------------------------------------------------+
|                  EdgeBox VPN Agent                        |
+-----------------------------------------------------------+
|                                                           |
|  [1] Registration Service                                 |
|   - Registers agent with Control Plane (/register)        |
|   - Sends heartbeats (/heartbeat)                         |
|   - Receives config updates (peers, modules)              |
|                                                           |
|  [2] CLI / Console UI                                     |
|   - Register/Unregister agent                             |
|   - Show active peers                                     |
|   - Enable/Disable modules                                |
|                                                           |
|  [3] Config & Module Manager                              |
|   - Stores current config                                 |
|   - Loads/unloads modules dynamically (OTA updates)       |
|   - Manages module lifecycle                              |
|                                                           |
|  [4] Pipeline                                             |
|    ┌───────────────────────────────┐                      |
|    │  Pre-VPN Modules              │                      |
|    │   (Firewall, IDS, GeoIP)      │                      |
|    └───────────────────────────────┘                      |
|             ↓                                             |
|    ┌───────────────────────────────┐                      |
|    │  In-VPN Modules               │                      |
|    │   (Traffic Logging, QoS)      │                      |
|    └───────────────────────────────┘                      |
|             ↓                                             |
|    ┌───────────────────────────────┐                      |
|    │  Post-VPN Modules             │                      |
|    │   (DLP, DNS Filtering)        │                      |
|    └───────────────────────────────┘                      |
|                                                           |
|  [5] WireGuard Interface (wg0 TUN)                        |
|   - Handles packet send/receive                           |
|   - Provides hook points for Pre- and Post-VPN modules    |
|                                                           |
+-----------------------------------------------------------+
```
## 1. Purpose

The EdgeBox VPN Agent runs on peer machines and connects to the Control Plane to form a secure VPN network.

- **MVP (star topology):** All peers connect to the server.
- **Future:** Full mesh topology will be supported.

**The agent:**
- Registers itself with the Control Plane
- Receives peer list and module assignments
- Handles VPN traffic through WireGuard
- Loads optional modules (Pre-VPN, In-VPN, Post-VPN) dynamically

---

## 2. Core Components

### 2.1 Registration Service

- Registers the agent with the Control Plane via `/register` API
- Sends periodic `/heartbeat` with:
  - Status (online/offline)
  - Version and uptime
  - Active modules
- Receives config updates (peer list, module activation)

### 2.2 CLI (Console UI)

Local management tool for:
- Register/unregister agent
- Show active peers
- Enable/disable modules locally

*CLI will adapt to OS environment (Bash on Linux, PowerShell on Windows).*

### 2.3 Config & Module Manager

- Stores current configuration (peers, ACLs, module assignments)
- Downloads and manages OTA module updates from Control Plane
- Starts/stops modules based on Control Plane instructions
- Manages module lifecycle without restarting the agent

### 2.4 Pipeline

Processes packets through modular stages:

- **Pre-VPN modules:**  
  Operate on plaintext packets before WireGuard encryption  
  _Examples: Firewall, IDS/IPS, GeoIP filtering_

- **In-VPN modules:**  
  Operate on encrypted packets inside the tunnel  
  _Examples: Traffic logging, QoS, anomaly detection_

- **Post-VPN modules:**  
  Operate on plaintext packets after WireGuard decryption  
  _Examples: DLP, DNS filtering, traffic analytics_

Each stage can have N active modules controlled by the Control Plane.

### 2.5 WireGuard Interface

- Uses TUN interface (`wg0`) to send and receive packets
- Handles plain IP packets before/after encryption
- Pipeline hooks into TUN for Pre-VPN and Post-VPN stages

---

## 3. Deployment on Linux

Agent runs as a systemd service for autostart and crash recovery.

**Example service file:**
```ini
[Unit]
Description=EdgeBox VPN Agent
After=network.target

[Service]
ExecStart=/usr/bin/edgebox-agent
Restart=always
RestartSec=5
User=edgebox
CapabilityBoundingSet=CAP_NET_ADMIN CAP_NET_RAW
AmbientCapabilities=CAP_NET_ADMIN CAP_NET_RAW

[Install]
WantedBy=multi-user.target
```

- **Config location:** `/etc/edgebox-agent/config.yaml`
- **Logs:** `journalctl -u edgebox-agent`

---

## 4. Cross-Platform Considerations

- **Shared code:** Registration Service, CLI, Config & Module Manager
- **Platform-specific code:**
  - WireGuard integration (kernel module or wireguard-go)
  - Packet pipeline hooks (TUN device handling)
  - Windows/macOS: Later support with OS-specific pipeline drivers (e.g. wireguard-nt for Windows, utun for macOS)

---

## 5. Docker Usage

- **Development:**  
  Docker can be used to simulate multiple agents for testing

- **Production:**  
  Agent should run on host OS (not Docker) to directly manage