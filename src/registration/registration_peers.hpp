#pragma once

#include "common/error_codes.hpp"
#include "common/peer_types.hpp"
#include "platform/common/tunnel_manager.hpp"
#include "registration/rest_client.hpp"

#include <atomic>
#include <memory>
#include <vector>

///
/// @brief Control Plane registration and heartbeat service
///
class registration_peers
{
public:
  ///
  /// @brief Constructor
  /// @param control_plane_url Optional Control Plane base URL
  ///
  explicit registration_peers(const std::string& control_plane_url = "");

  ///
  /// @brief Destructor
  ///
  ~registration_peers();

public:
  ///
  /// @brief Run periodic heartbeat loop to Control Plane
  /// @param running Atomic flag to control loop execution
  /// @param tunnel_mgr Tunnel manager instance used to apply peer updates
  ///
  auto heartbeat(const std::atomic<bool>& running, std::unique_ptr<platform::tunnel_manager>& tunnel_mgr) -> void;

  ///
  /// @brief Register with Control Plane and start VPN tunnel
  /// @param tunnel_mgr Tunnel manager instance (injected dependency)
  /// @return error_code indicating success or failure
  ///
  auto vpn_up(std::unique_ptr<platform::tunnel_manager>& tunnel_mgr) -> common::error_code;

private:
  ///
  /// @brief Re-register with Control Plane using existing keys and update peers
  ///
  /// Called when the heartbeat detects that this agent has been evicted from the
  /// server (e.g., TTL expired because the server returned 304 without updating
  /// last_seen). Does not regenerate keys or recreate the WireGuard interface.
  ///
  /// @param tunnel_mgr Tunnel manager instance used to apply peer updates
  /// @return error_code indicating success or failure
  ///
  auto re_register(std::unique_ptr<platform::tunnel_manager>& tunnel_mgr) -> common::error_code;
  std::unique_ptr<common::self_peer_t> self_peer_; ///< Cached peer information from Control Plane
  std::unique_ptr<rest_client> rest_client_;       ///< REST client for Control Plane API
};
