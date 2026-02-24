#include "registration/registration_peers.hpp"
#include "logging/logger.hpp"
#include "platform/common/platform_utils.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

static constexpr auto channel = "registration_peers";

///
///
registration_peers::registration_peers(const std::string& control_plane_url)
  : self_peer_{std::make_unique<common::self_peer_t>()}
  , rest_client_{
      control_plane_url.empty() ? std::make_unique<rest_client>() : std::make_unique<rest_client>(control_plane_url)}
{
}

///
///
registration_peers::~registration_peers() = default;

///
///
auto registration_peers::heartbeat(
  const std::atomic<bool>& running, std::unique_ptr<platform::tunnel_manager>& tunnel_mgr) -> void
{
  while(running)
  {
    try
    {
      auto ping_result = rest_client_->ping();
      if(ping_result)
      {
        self_peer_->self.last_seen = ping_result->self.last_seen;
        self_peer_->self.registered_at = ping_result->self.registered_at;
        self_peer_->self.vpn_ip = ping_result->self.vpn_ip;
        self_peer_->peers = std::move(ping_result->peers);
        self_peer_->ttl = ping_result->ttl;

        auto status = tunnel_mgr->update_peers(self_peer_->peers);
        if(status != common::error_code::success)
        {
          ERROR(channel, "Heartbeat peer update failed: {}", static_cast<int>(status));
        }

        DEBUG(channel, "Heartbeat successful, last seen: {}", self_peer_->self.last_seen);
      }
      else
      {
        DEBUG(channel, "No response from server, retrying...");
      }
    }
    catch(const std::exception& e)
    {
      ERROR(channel, "Heartbeat failed: {}", e.what());
    }

    if(!rest_client_->is_registered())
    {
      INFO(channel, "Heartbeat: agent was evicted from server, re-registering...");
      if(re_register(tunnel_mgr) != common::error_code::success)
      {
        WARN(channel, "Heartbeat: re-registration failed, will retry next interval");
      }
    }

    auto heartbeat_interval = std::max(1, self_peer_->ttl - 1);
    std::this_thread::sleep_for(std::chrono::seconds(heartbeat_interval));
  }
}

///
///
auto registration_peers::vpn_up(std::unique_ptr<platform::tunnel_manager>& tunnel_mgr) -> common::error_code
{
  DEBUG(channel, "Starting VPN setup with injected tunnel_manager...");
  auto status = common::error_code::success;

  try
  {
    self_peer_->self.hostname = platform::platform_utils::get_hostname();
    self_peer_->self.endpoint = platform::platform_utils::get_host_ip();

    status = tunnel_mgr->generate_keys();
    if(status != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to generate WireGuard keys: {}", static_cast<int>(status));
      return status;
    }

    self_peer_->self.pub_key = tunnel_mgr->get_public_key();

    if(self_peer_->self.endpoint_port == 0 || self_peer_->self.endpoint_port > 65535)
    {
      ERROR(channel, "vpn_up: Invalid endpoint port: {}", self_peer_->self.endpoint_port);
      return common::error_code::invalid_endpoint_format;
    }

    common::self_peer_t peers_resp;
    auto register_result = rest_client_->register_peer(
      self_peer_->self.pub_key,
      std::format("{}:{}", self_peer_->self.endpoint, self_peer_->self.endpoint_port),
      self_peer_->self.hostname,
      peers_resp);
    if(register_result != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to register with control plane: {}", static_cast<int>(register_result));
      return register_result;
    }

    self_peer_->self.vpn_ip = peers_resp.self.vpn_ip;
    self_peer_->self.last_seen = peers_resp.self.last_seen;
    self_peer_->self.registered_at = peers_resp.self.registered_at;
    self_peer_->peers = std::move(peers_resp.peers);
    self_peer_->ttl = peers_resp.ttl;

    status = tunnel_mgr->create_interface("wg0", self_peer_->self.vpn_ip, self_peer_->self.endpoint_port);
    if(status != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to create WireGuard interface: {}", static_cast<int>(status));
      return status;
    }

    status = tunnel_mgr->update_peers(self_peer_->peers);
    if(status != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to configure peers: {}", static_cast<int>(status));
      return status;
    }

    status = tunnel_mgr->start_tunnel();
    if(status != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to start WireGuard tunnel: {}", static_cast<int>(status));
      return status;
    }

    // Legacy TODO was for system_info::set_interface_ip - now handled by tunnel_manager
    // LEGACY CODE (commented out):
    // TODO:
    // status = platform::system_info::set_interface_ip(tunnel_manager_->inface_name(), self_peer_->self.vpn_ip);
    // if (status != common::error_code::success)
    // {
    //   ERROR(channel, "vpn_up: Failed to set interface IP: {}", static_cast<int>(status));
    //   return status;
    // }
  }
  catch(const std::system_error& e)
  {
    ERROR(channel, "Failed to vpn_up: {}", e.what());
    return common::error_code::unknown_error;
  }

  DEBUG(channel, "Hostname: {}", self_peer_->self.hostname);
  DEBUG(channel, "Host IP: {}", self_peer_->self.endpoint);
  DEBUG(channel, "VPN setup completed successfully");
  return status;
}

///
///
auto registration_peers::re_register(std::unique_ptr<platform::tunnel_manager>& tunnel_mgr) -> common::error_code
{
  INFO(channel, "Re-registering with Control Plane using existing keys...");

  if(self_peer_->self.pub_key.empty())
  {
    ERROR(channel, "re_register: no existing public key, cannot re-register without calling vpn_up first");
    return common::error_code::unknown_error;
  }

  auto peers_resp = common::self_peer_t{};
  auto register_result = rest_client_->register_peer(
    self_peer_->self.pub_key,
    std::format("{}:{}", self_peer_->self.endpoint, self_peer_->self.endpoint_port),
    self_peer_->self.hostname,
    peers_resp);

  if(register_result != common::error_code::success)
  {
    ERROR(channel, "re_register: failed to re-register: {}", static_cast<int>(register_result));
    return register_result;
  }

  self_peer_->self.vpn_ip = peers_resp.self.vpn_ip;
  self_peer_->self.last_seen = peers_resp.self.last_seen;
  self_peer_->self.registered_at = peers_resp.self.registered_at;
  self_peer_->peers = std::move(peers_resp.peers);
  self_peer_->ttl = peers_resp.ttl;

  auto status = tunnel_mgr->update_peers(self_peer_->peers);
  if(status != common::error_code::success)
  {
    ERROR(channel, "re_register: failed to update peers after re-registration: {}", static_cast<int>(status));
    return status;
  }

  INFO(channel, "Re-registration successful, {} peers updated", self_peer_->peers.size());
  return common::error_code::success;
}
