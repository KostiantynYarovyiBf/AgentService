#pragma once

#include "common/error_codes.hpp"
#include "common/peer_types.hpp"

#include <string>
#include <vector>

namespace platform
{

///
/// @brief Simplified tunnel manager using userspace WireGuard tools
///
/// This class provides a cross-platform solution for managing WireGuard tunnels
/// without requiring kernel module access. It uses the standard WireGuard
/// userspace tools (wg, wg-quick) which work on both Linux and macOS.
///
class tunnel_manager_base
{
public: // Constructors
  tunnel_manager_base();
  virtual ~tunnel_manager_base();

public: // Interface management
  ///
  /// @brief Create and configure a WireGuard interface
  /// @param interface_name Name of the interface (e.g., "wg0")
  /// @param vpn_ip IP address for this peer (e.g., "10.0.0.1/24")
  /// @param listen_port Port to listen on (default: 51820)
  /// @return error_code indicating success or failure
  ///
  virtual auto
  create_interface(const std::string& interface_name, const std::string& vpn_ip, std::uint16_t listen_port = 51820)
    -> common::error_code = 0;

  ///
  /// @brief Start the WireGuard tunnel
  /// @return error_code indicating success or failure
  ///
  virtual auto start_tunnel() -> common::error_code = 0;

  ///
  /// @brief Stop the WireGuard tunnel
  /// @return error_code indicating success or failure
  ///
  virtual auto stop_tunnel() -> common::error_code = 0;

public: // Peer management
  ///
  /// @brief Add a peer to the tunnel
  /// @param peer Peer information including public key and endpoint
  /// @return error_code indicating success or failure
  ///
  auto add_peer(const common::neighbor_peer_t& peer) -> common::error_code;

  ///
  /// @brief Remove a peer from the tunnel
  /// @param peer Peer to remove
  /// @return error_code indicating success or failure
  ///
  auto remove_peer(const common::neighbor_peer_t& peer) -> common::error_code;

  ///
  /// @brief Update all peers in the tunnel
  /// @param peers Vector of all peers that should be connected
  /// @return error_code indicating success or failure
  ///
  auto update_peers(const std::vector<common::neighbor_peer_t>& peers) -> common::error_code;

public: // Key management
  ///
  /// @brief Generate new WireGuard key pair
  /// @return error_code indicating success or failure
  ///
  virtual auto generate_keys() -> common::error_code = 0;

  ///
  /// @brief Get the public key
  /// @return Public key as string
  ///
  auto get_public_key() const -> std::string;

  ///
  /// @brief Get the private key
  /// @return Private key as string
  ///
  auto get_private_key() const -> std::string;

public: // Status
  ///
  /// @brief Check if tunnel is active
  /// @return true if tunnel is running
  ///
  auto is_active() const -> bool;

  ///
  /// @brief Get interface name
  /// @return Interface name
  ///
  auto get_interface_name() const -> std::string;

  ///
  /// @brief Get listen port
  /// @return Port number
  ///
  auto get_listen_port() const -> std::uint16_t;

public: // Host info
  ///
  /// @brief Get endpoint (IP:PORT format for WireGuard)
  /// @param port Port number to append to IP
  /// @return Endpoint in format "IP:PORT"
  ///
  auto get_endpoint(std::uint16_t port = 51820) -> std::string;

protected: // Helper methods
  ///
  /// @brief Create WireGuard configuration file
  /// @return error_code indicating success or failure
  ///
  virtual auto create_config_file() -> common::error_code = 0;

  ///
  /// @brief Get path to configuration file
  /// @return Path to config file
  ///
  virtual auto get_wireguard_config_path() const -> std::string = 0;

  ///
  /// @brief Get WireGuard configuration directory
  /// @return Path where WireGuard configs should be stored
  ///
  virtual auto get_wireguard_config_dir() const -> std::string = 0;

  ///
  /// @brief Calculate peer differences between current and new peer lists
  /// @param new_peers New peer list to compare against
  /// @param to_add Output vector of peers to add
  /// @param to_remove Output vector of peers to remove
  ///
  virtual auto compute_peer_diff(
    const std::vector<common::neighbor_peer_t>& new_peers,
    std::vector<common::neighbor_peer_t>& to_add,
    std::vector<common::neighbor_peer_t>& to_remove) -> void = 0;

  ///
  /// @brief Add peer dynamically to running tunnel using wg command
  /// @param peer Peer to add
  /// @return error_code indicating success or failure
  ///
  virtual auto wg_add_peer_dynamic(const common::neighbor_peer_t& peer) -> common::error_code = 0;

  ///
  /// @brief Remove peer dynamically from running tunnel using wg command
  /// @param peer Peer to remove
  /// @return error_code indicating success or failure
  ///
  virtual auto wg_remove_peer_dynamic(const common::neighbor_peer_t& public_key) -> common::error_code = 0;

protected: // Member variables
  std::string interface_name_;
  std::string private_key_;
  std::string public_key_;
  std::string vpn_ip_;
  std::uint16_t listen_port_;
  std::vector<common::neighbor_peer_t> peers_;
  bool is_active_;
};

} // namespace platform