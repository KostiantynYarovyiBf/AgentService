# EdgeBox VPN Agent

This is the EdgeBox VPN Agent project.  
Documentation can be found in the `docs/` folder.

## Requirements

For Linux, install the following libraries (see `requirements.txt`):

- `libcurl4-openssl-dev`
- `libssl-dev`
- `wireguard-tools`
- `libmnl-dev`

> **Note:** For other operating systems (macOS, Windows), these dependencies are not required or are replaced by platform-specific mocks or implementations.

## Building

To build the project with CMake:

### Linux

**Debug build:**
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_SYSTEM_NAME=Linux
cmake --build build
```

**Release build:**
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Linux
cmake --build build
cd build
cpack

## install
sudo dpkg -i <file.deb>
```

**Debug app with vscode:**
```Linux
sudo gdbserver  localhost:2345 ./build/AgentService
```

### macOS

**Debug build:**
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_SYSTEM_NAME=Darwin
cmake --build build
```

**Release build:**
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Darwin
cmake --build build
```

**Debug app with vscode:**
``` MacOs
sudo debugserver localhost:2345 ./build/AgentService
```

**Run with elevated privileges (if needed):**
```sh
sudo ./build/AgentService
```

## Integration Testing

Linux integration tests spin up two network namespaces, a Python mock control plane, and two agent instances to verify end-to-end WireGuard peer exchange.

See [`tests/integration/linux/README.md`](tests/integration/linux/README.md) for setup, topology, and run instructions.

## Control Plane Mock

A stateful Python mock server (`control_plane_mock/mock_server.py`) implements the full Control Plane API (`/register`, `/ping`) for local development and integration testing.

See [`control_plane_mock/README.md`](control_plane_mock/README.md) for usage and curl examples.

## License

This project is licensed under the Apache License 2.0. See the [LICENSE](LICENSE) file for details.
