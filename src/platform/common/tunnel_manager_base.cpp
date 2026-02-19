#include "platform/common/tunnel_manager_base.hpp"
#include "logging/logger.hpp"
#include "platform/common/platform_utils.hpp"

#include <algorithm>

static constexpr auto channel = "tunnel_manager";
constexpr auto key_preview_length = 8;

namespace platform
{
///
///
tunnel_manager_base::tunnel_manager_base()
  : interface_name_{"wg0"}
  , listen_port_{51820}
  , is_active_{false}
{
}

///
///
tunnel_manager_base::~tunnel_manager_base()
{
}

///
///
auto tunnel_manager_base::add_peer(const common::neighbor_peer_t& peer) -> common::error_code
{
  INFO(channel, "Adding peer: {} ({})", peer.hostname, peer.pub_key.substr(0, key_preview_length) + "...");
  auto status = common::error_code::success;
  auto it = std::find_if(
    peers_.begin(),
    peers_.end(),
    [&peer](const common::neighbor_peer_t& existing_peer) { return existing_peer.pub_key == peer.pub_key; });

  if(it != peers_.end())
  {
    WARN(channel, "Peer already exists, updating instead");
    *it = peer;
  }
  else
  {
    peers_.push_back(peer);
  }
  status = wg_add_peer_dynamic(peer);
  if(status != common::error_code::success)
  {
    ERROR(channel, "Failed to add peer");
    return status;
  }

  INFO(channel, "Peer added successfully");
  return status;
}

///
///
auto tunnel_manager_base::remove_peer(const common::neighbor_peer_t& peer) -> common::error_code
{
  INFO(channel, "Removing peer: {}...", peer.pub_key.substr(0, key_preview_length));

  auto original_size = peers_.size();
  peers_.erase(
    std::remove_if(
      peers_.begin(), peers_.end(), [&peer](const common::neighbor_peer_t& p) { return p.pub_key == peer.pub_key; }),
    peers_.end());

  if(peers_.size() == original_size)
  {
    WARN(channel, "Peer not found in peer list");
    return common::error_code::success;
  }

  auto status = create_config_file();
  if(status != common::error_code::success)
  {
    ERROR(channel, "Failed to update configuration file");
    return status;
  }

  if(is_active_)
  {
    status = wg_remove_peer_dynamic(peer);
    if(status != common::error_code::success)
    {
      ERROR(channel, "Failed to remove peer from active tunnel");
      return status;
    }
  }

  INFO(channel, "Peer removed successfully");
  return status;
}

///
///
auto tunnel_manager_base::update_peers(const std::vector<common::neighbor_peer_t>& peers) -> common::error_code
{
  INFO(channel, "Updating all peers, count: {}", peers.size());
  auto status = common::error_code::success;

  auto peers_to_add = std::vector<common::neighbor_peer_t>{};
  auto peers_to_remove = std::vector<common::neighbor_peer_t>{};

  compute_peer_diff(peers, peers_to_add, peers_to_remove);

  INFO(channel, "Peers diff - to remove: {}, to add: {}", peers_to_remove.size(), peers_to_add.size());

  for(const auto& peer : peers_to_remove)
  {
    status = remove_peer(peer);
    if(status != common::error_code::success)
    {
      ERROR(channel, "Failed to remove peer during update");
      return status;
    }
  }

  for(const auto& peer : peers_to_add)
  {
    status = add_peer(peer);
    if(status != common::error_code::success)
    {
      ERROR(channel, "Failed to add peer during update");
      return status;
    }
  }

  INFO(channel, "All peers updated successfully");
  return status;
}

///
///
auto tunnel_manager_base::get_public_key() const -> std::string
{
  return public_key_;
}

///
///
auto tunnel_manager_base::get_private_key() const -> std::string
{
  return private_key_;
}

///
///
auto tunnel_manager_base::is_active() const -> bool
{
  return is_active_;
}

///
///
auto tunnel_manager_base::get_interface_name() const -> std::string
{
  return interface_name_;
}

///
///
auto tunnel_manager_base::get_listen_port() const -> std::uint16_t
{
  return listen_port_;
}

///
///
auto tunnel_manager_base::get_endpoint(std::uint16_t port) -> std::string
{
  return platform_utils::get_host_ip() + ":" + std::to_string(port);
}

} // namespace platform