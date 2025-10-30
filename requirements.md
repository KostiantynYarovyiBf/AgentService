# Dependencies Requirements

## 🌍 Cross-platform libraries 
> **Platforms:** 🟢 **Linux** | 🟢 **macOS** | 🟢 **Windows**

| Library | Linux | macOS (Homebrew) | Windows (vcpkg/conan) |
|---------|-------|------------------|----------------------|
| `libssl-dev` | ✅ `libssl-dev` | ✅ `openssl@3` | ✅ `openssl` |
| `wireguard-tools` | ✅ `wireguard-tools` | ✅ `wireguard-tools` | ✅ `wireguard-nt` |
| `nlohmann-json3-dev` | ✅ `nlohmann-json3-dev` | ✅ `nlohmann-json` | ✅ `nlohmann-json` (header-only) |
| `libboost-all-dev` | ✅ `libboost-all-dev` | ✅ `boost` | ✅ `boost` (native support) |
| `pkg-config` | ✅ `pkg-config` | ✅ `pkg-config` | ✅ `pkgconf` (via MSYS2/vcpkg) |

## ⚠️ Linux-only libraries 
> **Platforms:** 🟡 **Linux only** (Debian/Ubuntu format)

| Library | Purpose | Cross-platform Alternative |
|---------|---------|---------------------------|
| `libcurl4-openssl-dev` | HTTP client | **macOS:** `curl`, **Windows:** `curl[openssl]` |
| `libmnl-dev` | Kernel netlink library | **macOS:** BSD socket APIs, **Windows:** WinSock2/WinAPI |
| `libnl-3-dev` | Netlink library | **macOS:** Network framework, **Windows:** Windows networking APIs |
| `libnl-route-3-dev` | Netlink routing library | **macOS:** Network framework, **Windows:** Windows networking APIs |

---

## 📊 Library Usage Analysis (Cross-platform Files Only)

### ✅ **Currently Used Libraries**

#### 🟢 **cpr** (libcurl4-openssl-dev + libssl-dev)
**Purpose:** HTTP/REST API client  
**Files:**
- `src/registration/rest_client.cpp` - HTTP client implementation
- `src/registration/rest_client.hpp` - API declarations
- **Functions:** `cpr::Get()`, `cpr::Post()` for API calls

#### 🟢 **nlohmann-json3-dev** 
**Purpose:** JSON parsing and serialization  
**Files:**
- `src/config/config.hpp` - Configuration file parsing
- `src/config/config.cpp` - JSON file I/O operations  
- `src/registration/rest_client.cpp` - API response parsing
- `src/registration/rest_client.hpp` - JSON type declarations

#### 🟢 **libboost-all-dev** (minimal usage)
**Purpose:** Network utilities  
**Files:**
- `src/registration/registration_peers.hpp` - Only uses `boost/asio/ip/network_v4.hpp`

#### 🟢 **spdlog** (fetched via CMake)
**Purpose:** Structured logging  
**Files:**
- `src/logging/logger_core.cpp` - Logger initialization
- `src/logging/logger_core.hpp` - Logger interface  
- `src/logging/logger.hpp` - Logging macros (`DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`)
- **Usage:** Throughout codebase via macros

## 🎯 **Platform Compatibility Summary**

| Component | 🟢 **Linux** | 🟢 **macOS** | 🟢 **Windows** | Status |
|-----------|-------------|-------------|---------------|---------|
| **HTTP/REST API** | ✅ cpr/libcurl | ✅ cpr/curl | ✅ cpr/curl | 🟢 Cross-platform |
| **JSON Processing** | ✅ nlohmann-json | ✅ nlohmann-json | ✅ nlohmann-json | 🟢 Cross-platform |
| **Network Utils** | ✅ boost::asio | ✅ boost::asio | ✅ boost::asio | 🟢 Cross-platform |
| **Logging** | ✅ spdlog | ✅ spdlog | ✅ spdlog | 🟢 Cross-platform |
| **WireGuard Interface** | ✅ Native APIs | ✅ WireGuard app | ✅ WireGuard-NT | ⚠️ Platform-specific |
| **Network Management** | ✅ netlink/libnl | ❌ Need BSD APIs | ❌ Need WinAPI | 🟡 Linux-only |

### 📋 **Notes**
- **Core functionality** (HTTP API, JSON, logging) is fully cross-platform
- **Network management** requires platform-specific implementations
- **WireGuard** is abstracted through platform-specific interfaces
