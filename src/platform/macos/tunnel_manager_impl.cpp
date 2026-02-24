#include "platform/macos/tunnel_manager_impl.hpp"
#include "logging/logger.hpp"
#include "platform/common/platform_utils.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

static constexpr auto channel = "tunnel_manager_macos";
constexpr auto key_preview_length = 8;

namespace platform
{
///
///
tunnel_manager_impl::tunnel_manager_impl()
{
  INFO(channel, "Initializing macOS tunnel manager");

  if(generate_keys() != common::error_code::success)
  {
    WARN(channel, "Failed to generate WireGuard keys during initialization");
  }
}

///
///
tunnel_manager_impl::~tunnel_manager_impl()
{
  if(is_active_)
  {
    INFO(channel, "Cleaning up active tunnel");
    stop_tunnel();
  }
}

///
///
auto tunnel_manager_impl::create_interface(
  const std::string& interface_name, const std::string& vpn_ip, std::uint16_t listen_port) -> common::error_code
{
  INFO(channel, "Creating WireGuard interface: {}, IP: {}, Port: {}", interface_name, vpn_ip, listen_port);

  interface_name_ = interface_name;
  vpn_ip_ = vpn_ip;
  listen_port_ = listen_port;

  auto result = create_config_file();
  if(result != common::error_code::success)
  {
    ERROR(channel, "Failed to create configuration file");
    return result;
  }

  INFO(channel, "WireGuard interface configured successfully");
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::start_tunnel() -> common::error_code
{
  if(is_active_)
  {
    WARN(channel, "Tunnel is already active");
    return common::error_code::success;
  }

  INFO(channel, "Starting WireGuard tunnel: {}", interface_name_);

  auto output = std::string{};
  // TODO(launchd): remove sudo once agent runs as LaunchDaemon/root; call wg-quick directly.
  auto command = "sudo wg-quick up " + get_wireguard_config_path();

  auto result = execute_command(command, output);
  if(result != common::error_code::success)
  {
    ERROR(channel, "Failed to start tunnel: {}", output);
    return result;
  }

  is_active_ = true;
  INFO(channel, "WireGuard tunnel started successfully");
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::stop_tunnel() -> common::error_code
{
  if(!is_active_)
  {
    WARN(channel, "Tunnel is not active");
    return common::error_code::success;
  }

  INFO(channel, "Stopping WireGuard tunnel: {}", interface_name_);

  auto output = std::string{};
  // TODO(launchd): remove sudo once agent runs as LaunchDaemon/root; call wg-quick directly.
  auto command = "sudo wg-quick down " + get_wireguard_config_path();

  auto result = execute_command(command, output);
  if(result != common::error_code::success)
  {
    ERROR(channel, "Failed to stop tunnel: {}", output);
    return result;
  }

  is_active_ = false;
  INFO(channel, "WireGuard tunnel stopped successfully");
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::generate_keys() -> common::error_code
{
  INFO(channel, "Generating WireGuard key pair");

  auto output = std::string{};
  auto result = execute_command("wg genkey", output);
  if(result != common::error_code::success)
  {
    ERROR(channel, "Failed to generate private key - is WireGuard installed?");
    return result;
  }

  private_key_ = output;
  if(!private_key_.empty() && private_key_.back() == '\n')
  {
    private_key_.pop_back();
  }

  auto command = std::string{"echo '" + private_key_ + "' | wg pubkey"};
  result = execute_command(command, output);
  if(result != common::error_code::success)
  {
    ERROR(channel, "Failed to generate public key");
    return result;
  }

  public_key_ = output;
  if(!public_key_.empty() && public_key_.back() == '\n')
  {
    public_key_.pop_back();
  }

  INFO(channel, "Key pair generated successfully, public key: {}...", public_key_.substr(0, key_preview_length));
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::create_config_file() -> common::error_code
{
  auto config_path = get_wireguard_config_path();
  INFO(channel, "Creating configuration file: {}", config_path);

  std::filesystem::path dir = std::filesystem::path(config_path).parent_path();
  try
  {
    std::filesystem::create_directories(dir);
  }
  catch(const std::exception& e)
  {
    ERROR(channel, "Failed to create config directory {}: {}", dir.string(), e.what());
    return common::error_code::file_error;
  }

  auto config_file = std::ofstream(config_path);
  if(!config_file.is_open())
  {
    ERROR(channel, "Failed to create configuration file: {}", config_path);
    return common::error_code::file_error;
  }

  config_file << "[Interface]\n";
  config_file << "PrivateKey = " << private_key_ << "\n";
  config_file << "Address = " << vpn_ip_ << "\n";
  config_file << "ListenPort = " << listen_port_ << "\n";

  config_file << "# macOS specific settings\n";
  config_file << "# DNS = 8.8.8.8\n";
  config_file << "\n";

  for(const auto& peer : peers_)
  {
    config_file << "[Peer]\n";
    config_file << "PublicKey = " << peer.pub_key << "\n";

    if(!peer.endpoint.empty())
    {
      config_file << "Endpoint = " << peer.endpoint << "\n";
    }

    if(!peer.allowed_vpn_ips.empty())
    {
      config_file << "AllowedIPs = ";
      for(size_t i = 0; i < peer.allowed_vpn_ips.size(); ++i)
      {
        if(i > 0)
        {
          config_file << ", ";
        }
        config_file << peer.allowed_vpn_ips[i];
      }
      config_file << "\n";
    }

    config_file << "PersistentKeepalive = 25\n";
    config_file << "\n";
  }

  config_file.close();
  INFO(channel, "Configuration file created successfully with {} peers", peers_.size());
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::compute_peer_diff(
  const std::vector<common::neighbor_peer_t>& new_peers,
  std::vector<common::neighbor_peer_t>& to_add,
  std::vector<common::neighbor_peer_t>& to_remove) -> void
{
  for(const auto& current_peer : peers_)
  {
    auto found = std::find_if(
      new_peers.begin(),
      new_peers.end(),
      [&current_peer](const common::neighbor_peer_t& p) { return p.pub_key == current_peer.pub_key; });

    if(found == new_peers.end())
    {
      to_remove.push_back(current_peer);
    }
  }

  for(const auto& new_peer : new_peers)
  {
    auto found = std::find_if(
      peers_.begin(),
      peers_.end(),
      [&new_peer](const common::neighbor_peer_t& p) { return p.pub_key == new_peer.pub_key; });

    if(found == peers_.end())
    {
      to_add.push_back(new_peer);
    }
  }
}

///
///
auto tunnel_manager_impl::wg_add_peer_dynamic(const common::neighbor_peer_t& peer) -> common::error_code
{
  auto output = std::string{};
  auto cmd = std::ostringstream{};
  // TODO(launchd): remove sudo once agent runs as LaunchDaemon/root; call wg directly.
  cmd << "sudo wg set " << interface_name_ << " peer " << peer.pub_key;

  if(!peer.endpoint.empty())
  {
    cmd << " endpoint " << peer.endpoint;
  }

  if(!peer.allowed_vpn_ips.empty())
  {
    cmd << " allowed-ips ";
    for(size_t i = 0; i < peer.allowed_vpn_ips.size(); ++i)
    {
      if(i > 0)
      {
        cmd << ",";
      }
      cmd << peer.allowed_vpn_ips[i];
    }
  }

  auto result = execute_command(cmd.str(), output);
  if(result != common::error_code::success)
  {
    WARN(channel, "Failed to add peer dynamically, configuration updated for next restart");
    return result;
  }

  for(const auto& ip : peer.allowed_vpn_ips)
  {
    auto route_output = std::string{};
    // TODO(launchd): remove sudo once agent runs as LaunchDaemon/root.
    auto route_cmd = "sudo route -q -n add -net " + ip + " -interface " + interface_name_;
    if(execute_command(route_cmd, route_output) != common::error_code::success)
    {
      WARN(channel, "Failed to add route {} via {}: {}", ip, interface_name_, route_output);
    }
    else
    {
      INFO(channel, "Route {} -> {} added", ip, interface_name_);
    }
  }

  INFO(channel, "Peer added dynamically to running tunnel");
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::wg_remove_peer_dynamic(const common::neighbor_peer_t& public_key) -> common::error_code
{
  auto output = std::string{};
  // TODO(launchd): remove sudo once agent runs as LaunchDaemon/root; call wg directly.
  auto command = "sudo wg set " + interface_name_ + " peer " + public_key.pub_key + " remove";

  auto result = execute_command(command, output);
  if(result != common::error_code::success)
  {
    WARN(channel, "Failed to remove peer dynamically, configuration updated for next restart");
    return result;
  }

  for(const auto& ip : public_key.allowed_vpn_ips)
  {
    auto route_output = std::string{};
    // TODO(launchd): remove sudo once agent runs as LaunchDaemon/root.
    auto route_cmd = "sudo route -q -n delete -net " + ip + " -interface " + interface_name_;
    if(execute_command(route_cmd, route_output) != common::error_code::success)
    {
      WARN(channel, "Failed to remove route {} via {}: {}", ip, interface_name_, route_output);
    }
    else
    {
      INFO(channel, "Route {} -> {} removed", ip, interface_name_);
    }
  }

  INFO(channel, "Peer removed dynamically from running tunnel");
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::get_wireguard_config_path() const -> std::string
{
  return get_wireguard_config_dir() + interface_name_ + ".conf";
}

///
///
auto tunnel_manager_impl::get_wireguard_config_dir() const -> std::string
{
  return platform_utils::get_home_dir() + "/.config/wireguard/";
}

///
///
auto tunnel_manager_impl::execute_command(const std::string& command, std::string& output) -> common::error_code
{
  auto buffer = std::array<char, 128>{};
  auto result = std::string{};

  auto pipe_deleter = [](FILE* f)
  {
    if(f)
    {
      pclose(f);
    }
  };
  std::unique_ptr<FILE, decltype(pipe_deleter)> pipe(popen(command.c_str(), "r"), pipe_deleter);
  if(!pipe)
  {
    return common::error_code::system_error;
  }

  while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
  {
    result += buffer.data();
  }

  if(pclose(pipe.release()) != 0)
  {
    output = result;
    return common::error_code::system_error;
  }

  output = result;
  return common::error_code::success;
}

} // namespace platform