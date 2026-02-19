#pragma once

#include "platform/common/tunnel_manager_base.hpp"

#include "common/error_codes.hpp"
#include "common/peer_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace platform
{

///
///
///
class tunnel_manager_impl : public tunnel_manager_base
{
public: // Constructors
  tunnel_manager_impl();
  ~tunnel_manager_impl();

public: // Interface management
  ///
  /// \see tunnel_manager_base::create_interface
  ///
  auto create_interface(const std::string& interface_name, const std::string& vpn_ip, std::uint16_t listen_port = 51820)
    -> common::error_code override;

  ///
  /// \see tunnel_manager_base::start_tunnel
  ///
  auto start_tunnel() -> common::error_code override;

  ///
  /// \see tunnel_manager_base::stop_tunnel
  ///
  auto stop_tunnel() -> common::error_code override;

public: // Key management
  ///
  /// \see tunnel_manager_base::generate_keys
  ///
  auto generate_keys() -> common::error_code override;

private: // Wg operations
  ///
  /// @brief Create WireGuard configuration file
  /// @return error_code indicating success or failure
  ///
  auto create_config_file() -> common::error_code override;

  ///
  /// @brief Get path to configuration file
  /// @return Path to config file
  ///
  auto get_wireguard_config_path() const -> std::string override;

  ///
  /// @brief Get WireGuard configuration directory
  /// @return Path where WireGuard configs should be stored
  ///
  auto get_wireguard_config_dir() const -> std::string override;

  ///
  /// @brief Calculate peer differences between current and new peer lists
  /// @param new_peers New peer list to compare against
  /// @param to_add Output vector of peers to add
  /// @param to_remove Output vector of peers to remove
  ///
  auto compute_peer_diff(
    const std::vector<common::neighbor_peer_t>& new_peers,
    std::vector<common::neighbor_peer_t>& to_add,
    std::vector<common::neighbor_peer_t>& to_remove) -> void override;

  ///
  /// @brief Add peer dynamically to running tunnel using wg command
  /// @param peer Peer to add
  /// @return error_code indicating success or failure
  ///
  auto wg_add_peer_dynamic(const common::neighbor_peer_t& peer) -> common::error_code override;

  ///
  /// @brief Remove peer dynamically from running tunnel using wg command
  /// @param peer Peer to remove
  /// @return error_code indicating success or failure
  ///
  auto wg_remove_peer_dynamic(const common::neighbor_peer_t& public_key) -> common::error_code override;

private: // Helper methods
  ///
  /// @brief Execute shell command and capture output
  /// @param command Command to execute
  /// @param output Reference to store command output
  /// @return error_code indicating success or failure
  ///
  auto execute_command(const std::string& command, std::string& output) -> common::error_code;
};

} // namespace platform