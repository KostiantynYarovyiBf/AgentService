
#pragma once
#include "common/error_codes.hpp"
#include "config/config.hpp"
#include "platform/common/tunnel_manager.hpp"
#include "registration/registration_peers.hpp"

#include <atomic>
#include <memory>
#include <thread>

///
/// @brief Main VPN agent service orchestrator
///
class agent_service
{
public:
  ///
  /// @brief Constructor - initializes all service components
  /// @param control_plane_url Optional Control Plane base URL
  ///
  explicit agent_service(const std::string& control_plane_url = "");

  ///
  /// @brief Destructor - stops service and cleans up resources
  ///
  ~agent_service();

  ///
  /// @brief Start the agent service and all background operations
  ///
  auto start() -> void;

  ///
  /// @brief Stop the agent service and wait for threads to complete
  ///
  auto stop() -> void;

private:
  std::unique_ptr<config> config_;                           ///< Configuration manager
  std::unique_ptr<registration_peers> reg_service_;          ///< Control Plane registration service
  std::unique_ptr<platform::tunnel_manager> tunnel_manager_; ///< WireGuard tunnel manager

  std::vector<std::jthread> threads_; ///< Background operation threads
  std::atomic<bool> running_;         ///< Service running flag for thread coordination
};
