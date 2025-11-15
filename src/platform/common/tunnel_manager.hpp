#pragma once

#include "common/error_codes.hpp"
#include "common/peer_types.hpp"

#include <memory>

namespace platform
{

class tunnel_manager_impl;

///
/// TODO docstr
///
class tunnel_manager
{
public: // Constructors
  tunnel_manager();
  ~tunnel_manager();

public: // Interface management
  ///
  /// \see tunnel_manager_base::create_interface
  ///
  auto create_interface(const std::string& interface_name, const std::string& vpn_ip, std::uint16_t listen_port = 51820)
    -> common::error_code;

  ///
  /// \see tunnel_manager_base::start_tunnel
  ///
  auto start_tunnel() -> common::error_code;

  ///
  /// \see tunnel_manager_base::stop_tunnel
  ///
  auto stop_tunnel() -> common::error_code;

public: // Peer management
  ///
  /// \see tunnel_manager_base::add_peer
  ///
  auto add_peer(const common::neighbor_peer_t& peer) -> common::error_code;

  ///
  /// \see tunnel_manager_base::remove_peer
  ///
  auto remove_peer(const common::neighbor_peer_t& peer) -> common::error_code;

  ///
  /// \see tunnel_manager_base::update_peers
  ///
  auto update_peers(const std::vector<common::neighbor_peer_t>& peers) -> common::error_code;

public: // Key management
  ///
  /// \see tunnel_manager_base::generate_keys
  ///
  auto generate_keys() -> common::error_code;

  ///
  /// \see tunnel_manager_base::get_public_key
  ///
  auto get_public_key() const -> std::string;

  ///
  /// \see tunnel_manager_base::get_private_key
  ///
  auto get_private_key() const -> std::string;

public: // Status
  ///
  /// \see tunnel_manager_base::is_active
  ///
  auto is_active() const -> bool;

  ///
  /// \see tunnel_manager_base::get_interface_name
  ///
  auto get_interface_name() const -> std::string;

  ///
  /// \see tunnel_manager_base::get_listen_port
  ///
  auto get_listen_port() const -> std::uint16_t;

public: // Host info
  ///
  /// \see tunnel_manager_base::get_endpoint
  ///
  auto get_endpoint(std::uint16_t port = 51820) -> std::string;

private: // Member variables
  std::unique_ptr<tunnel_manager_impl> tunnel_manager_impl_;
};

} // namespace platform