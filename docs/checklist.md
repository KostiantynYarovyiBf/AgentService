# ✅ EdgeVPN Agent — MVP Checklist

This document outlines the minimum viable product (MVP) for the EdgeVPN Agent. The goal is to implement a stable foundation for a WireGuard-based peer agent, with a modular architecture and readiness for future features.

---

## 📡 Core Functionality

- [ ] Register peer with the Control Plane (`POST /register`)
- [ ] Wait for approval (polling or push)
- [ ] Receive configuration: allowed IP, port
- [ ] Setup WireGuard interface
- [ ] Connect to other peers in the mesh

---

## 🧱 Architecture

- [ ] Modular structure of the agent (by components)
- [ ] Include CLI, logger, config loader, REST client
- [ ] Each component (if needed) runs in its own thread
- [ ] Thread manager to handle lifecycle and clean shutdowns

---

## 🛠 Infrastructure and CI/CD

- [ ] Run agent as a systemd service (Linux)
- [ ] CI/CD with GitHub Actions:
  - [ ] build steps
  - [ ] unit tests
  - [ ] integration tests
- [ ] Docker-based integration tests (at least 2 peers + Control Plane)
- [ ] `.deb` packaging for installation

---

## 💻 Cross-Platform Design

- [ ] Clearly split shared and OS-specific code
- [ ] Provide wrappers for platform-dependent logic
- [ ] Make architecture extendable to Windows/macOS

---

## 🔄 Traffic Pipeline

- [ ] Implement basic pipeline: Pre-VPN → In-VPN → Post-VPN
- [ ] Handle traffic through each phase with basic logging
- [ ] Define API to attach modules in each phase
- [ ] No active modules yet — just a working skeleton

---

## ⚠ Error & Edge Case Handling

- [ ] Handle missing Control Plane responses
- [ ] Handle peer registration rejections
- [ ] Handle failed interface setup
- [ ] Log errors and status updates
- [ ] Cover with unit and integration tests

---

## 📁 Documentation

- [ ] `README.md` — overview, getting started, goals
- [ ] `docs/api.md` — Control Plane REST API specification
- [ ] `docs/architecture.md` — UML + flow diagrams
- [ ] `CONTRIBUTING.md` — how to build and add new modules

---

> Автор: @kyarovyi  
> Repo: https://github.com/KostiantynYarovyiBf/AgentService 
