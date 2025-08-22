#include "registration/registration_peers.hpp"
#include "logging/logger.hpp"

#include "wg_interface.hpp"
#include <system_info.hpp>

#include <thread>
#include <chrono>

static constexpr auto channel = "registration_peers";

///
///
registration_peers::registration_peers()
    : wg_interface_{std::make_unique<platform::wg_interface>()},
      self_peer_{std::make_unique<common::self_peer_t>()},
      rest_client_{std::make_unique<rest_client>()} {}

///
///
registration_peers::~registration_peers() = default;

///
///
auto registration_peers::heartbeat(const std::atomic<bool> &running) -> void
{
  while (running)
  {
    std::this_thread::sleep_for(std::chrono::seconds(self_peer_->ttl));
    try
    {
      auto ping_result = rest_client_->ping();
      if (ping_result)
      {
        self_peer_->self.last_seen = ping_result->self.last_seen;
        self_peer_->self.registered_at = ping_result->self.registered_at;
        self_peer_->self.vpn_ip = ping_result->self.vpn_ip;
        self_peer_->peers = std::move(ping_result->peers);
        self_peer_->ttl = ping_result->ttl;
        DEBUG(channel, "Heartbeat successful, last seen: {}",
              self_peer_->self.last_seen);
      }
      else
      {
        DEBUG(channel, "No response from server, retrying...");
      }
    }
    catch (const std::exception &e)
    {
      ERROR(channel, "Heartbeat failed: {}", e.what());
    }
  }
}

///
///
auto registration_peers::vpn_up() -> common::error_code
{
  DEBUG(channel, "Starting VPN setup...");
  auto status = common::error_code::success;

  try
  {
    self_peer_->self.hostname = platform::system_info::get_hostname();
    self_peer_->self.endpoint = platform::system_info::get_host_ip();

    status = wg_interface_->setup_wg();
    if (status != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to setup WireGuard interface: {}", static_cast<int>(status));
      return status;
    }

    self_peer_->self.pub_key = wg_interface_->pub_key();
    auto peers_resp = rest_client_->register_peer(self_peer_->self.pub_key,
                                                  std::format("{}:{}",
                                                              self_peer_->self.endpoint,
                                                              self_peer_->self.endpoint_port),
                                                  self_peer_->self.hostname);
    self_peer_->self.vpn_ip = peers_resp.self.vpn_ip;
    self_peer_->self.last_seen = peers_resp.self.last_seen;
    self_peer_->self.registered_at = peers_resp.self.registered_at;
    self_peer_->peers = std::move(peers_resp.peers);

    status = wg_interface_->up_wg_iface(self_peer_->self.vpn_ip);
    if (status != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to bring up WireGuard interface: {}", static_cast<int>(status));
      return status;
    }

    status = wg_interface_->setup_tunnel(self_peer_->peers);
    if (status != common::error_code::success)
    {
      ERROR(channel, "vpn_up: Failed to setup WireGuard tunnel: {}", static_cast<int>(status));
      return status;
    }

    // TODO:
    // status = platform::system_info::set_interface_ip(wg_interface_->inface_name(), self_peer_->self.vpn_ip);
    // if (status != common::error_code::success)
    // {
    //   ERROR(channel, "vpn_up: Failed to set interface IP: {}", static_cast<int>(status));
    //   return status;
    // }
  }
  catch (const std::system_error &e)
  {
    ERROR(channel, "Failed to vpn_up: {}", e.what());
    return common::error_code::unknown_error;
  }

  DEBUG(channel, "Hostname: {}", self_peer_->self.hostname);
  DEBUG(channel, "Host IP: {}", self_peer_->self.endpoint);
  return status;
}
