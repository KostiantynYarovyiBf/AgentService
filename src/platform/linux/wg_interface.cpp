#include "wg_interface.hpp"
#include "logging/logger.hpp"

extern "C"
{
#include <libs/wireguard/wireguard.h>
}

#include <array>
#include <cerrno>
#include <format>
#include <system_error>
#include <arpa/inet.h>

static constexpr auto channel = "wg_interface";
namespace platform
{

  ///
  ///@brief Converts a WireGuard key (wg_key) to a base64-encoded string.
  ///
  /// This utility function takes a 32-byte WireGuard key (public, private, or
  /// preshared) and encodes it as a base64 string using the WireGuard library's
  /// wg_key_to_base64 function. The resulting string is suitable for display,
  /// logging, or configuration file usage.
  ///
  ///@param key The WireGuard key to convert (array of 32 bytes).
  ///@return std::string The base64-encoded string representation of the key.
  ///
  auto wg_key_to_string(const wg_key &key) -> std::string
  {
    wg_key_b64_string base64;
    wg_key_to_base64(base64, key);
    return std::string(base64);
  }

  ///
  ///
  auto wg_interface::wg_device_deleter::operator()(wg_device *ptr) const -> void
  {
    if (ptr)
    {
      wg_free_device(ptr);
    }
  }

  ///
  ///
  wg_interface::wg_interface()
      : wg_device_{make_unique_wg_device()}
  {
  }

  ///
  ///
  wg_interface::~wg_interface() = default;

  ///
  ///
  auto wg_interface::setup_wg(std::uint16_t port, const char *wg_name) -> common::error_code
  {
    auto wg_status_code = 0;
    DEBUG(channel, "setup_wg: Initializing WireGuard setup for interface: '{}'",
          wg_name);

    strncpy(wg_device_->name, wg_name, sizeof(wg_device_->name) - 1);

    wg_key private_key, public_key;
    wg_generate_private_key(private_key);
    wg_generate_public_key(public_key, private_key);

    // Set keys in device struct
    memcpy(wg_device_->private_key, private_key, sizeof(wg_key));
    memcpy(wg_device_->public_key, public_key, sizeof(wg_key));

    wg_device_->listen_port = port;

    wg_device_->flags = static_cast<wg_device_flags>(
        static_cast<uint32_t>(wg_device_->flags) |
        static_cast<uint32_t>(WGDEVICE_HAS_PRIVATE_KEY) |
        static_cast<uint32_t>(WGDEVICE_HAS_PUBLIC_KEY) |
        static_cast<uint32_t>(WGDEVICE_HAS_LISTEN_PORT));
    wg_status_code = if_nametoindex(wg_name);
    if (wg_status_code != 0)
    {
      ERROR(
          channel,
          "setup_wg: WireGuard interface '{}' already exists, skipping creation. Wg status code '{}'",
          wg_name, wg_status_code);
      return common::error_code::device_exists;
    }
    wg_status_code = wg_add_device(wg_device_->name);
    if (wg_status_code != 0)
    {
      ERROR(
          channel,
          "setup_wg: WireGuard add device is failed '{}', Wg status code '{}'",
          wg_name, wg_status_code);
      return common::error_code::device_create_failed;
    }
    wg_status_code = wg_set_device(wg_device_.get());
    if (wg_status_code != 0)
    {
      ERROR(
          channel,
          "setup_wg: WireGuard set device is failed '{}', Wg status code '{}'",
          wg_name, wg_status_code);
      return common::error_code::device_configure_failed;
    }

    INFO(channel, "setup_wg: wg0 configured successfully");
    return common::error_code::success;
  }

  ///
  ///
  auto wg_interface::up_wg_iface(const std::string &ip, const char *wg_name) -> common::error_code
  {
    auto wg_status_code = 0;
    INFO(channel, "Bringing up WireGuard interface: '{}', ip '{}'", wg_name, ip);

    std::string ip_cmd = std::format("ip addr add '{}' dev '{}'", ip, wg_name);
    wg_status_code = std::system(ip_cmd.c_str());
    if (wg_status_code != 0)
    {
      ERROR(channel, "Failed to set IP address on '{}', wg status '{}'", wg_name, wg_status_code);
      return common::error_code::set_ip_failed;
    }

    std::string up_cmd = std::format("ip link set '{}' up", wg_name);
    wg_status_code = std::system(up_cmd.c_str());
    if (wg_status_code != 0)
    {
      ERROR(channel, "Failed to bring '{}' up (code: '{}')", wg_name, wg_status_code);
      return common::error_code::set_ip_failed;
    }
    INFO(channel, "WireGuard interface [{}] is up with ip [{}]", wg_name, ip);
    return common::error_code::success;
  }

  ///
  ///
  auto wg_interface::setup_tunnel(const std::vector<common::neighbor_peer_t> &peers) -> common::error_code
  {
    // sudo wg set wg0 peer PUBKEY_1 allowed-ips 10.0.0.2/32 endpoint IP1:PORT1
    INFO(channel, "setup_tunnel: Setup WireGuard peers");
    // Setup new peers topo. I cannot use smart ptr here cuz wg_free_device free all peers and allowed ips.
    // Another solution create new device and move from prev instance all except peers.
    free_peers();

    wg_peer *prev_peer = wg_device_->first_peer;
    if (prev_peer)
    {
      while (prev_peer->next_peer)
      {
        prev_peer = prev_peer->next_peer;
      }
    }

    for (const auto &p : peers)
    {
      wg_peer *peer = (wg_peer *)calloc(1, sizeof(wg_peer));

      if (wg_key_from_base64(peer->public_key, p.pub_key.c_str()) != 0)
      {
        ERROR(channel, "setup_tunnel: Failed to decode public key '{}'", p.pub_key);
        free(peer);
        return common::error_code::invalid_pub_key_format;
      }

      if (!endpoint_to_wg_endpoint(p.endpoint, peer->endpoint))
      {
        ERROR(channel, "setup_tunnel: Failed to convert endpoint '{}'", p.endpoint);
        free(peer);
        return common::error_code::invalid_endpoint_format;
      }

      convert_allowed_ips(p.allowed_vpn_ips, peer->first_allowedip, peer->last_allowedip);

      if (wg_device_->first_peer == nullptr)
      {
        wg_device_->first_peer = peer;
      }
      else
      {
        prev_peer->next_peer = peer;
      }
      prev_peer = peer;
    }

    INFO(channel, "setup_tunnel: Apply the configuration");
    if (wg_set_device(wg_device_.get()) != 0)
    {
      return common::error_code::device_configure_failed;
    }

    return common::error_code::success;
  }

  ///
  ///
  auto wg_interface::pub_key() const -> std::string
  {
    return wg_key_to_string(wg_device_->public_key);
  }

  ///
  ///
  auto wg_interface::priv_key() const -> std::string
  {
    return wg_key_to_string(wg_device_->private_key);
  }

  ///
  ///
  auto wg_interface::inface_name() const -> std::string { return wg_device_->name; }

  ///
  ///
  auto wg_interface::port_key() const -> std::uint16_t
  {
    return wg_device_->listen_port;
  }

  ///
  ///
  auto wg_interface::free_peers() -> void
  {
    wg_peer *peer = wg_device_->first_peer;
    while (peer)
    {
      wg_peer *next_peer = peer->next_peer;

      wg_allowedip *allowedip = peer->first_allowedip;
      while (allowedip)
      {
        wg_allowedip *next_allowedip = allowedip->next_allowedip;
        free(allowedip);
        allowedip = next_allowedip;
      }

      free(peer);
      peer = next_peer;
    }
    wg_device_->first_peer = nullptr;
    wg_device_->last_peer = nullptr;
  }

  ///
  ///
  auto wg_interface::endpoint_to_wg_endpoint(const std::string &endpoint_str, wg_endpoint &endpoint) -> bool
  {
    auto pos = endpoint_str.find(':');
    if (pos == std::string::npos)
    {
      ERROR(channel, "endpoint_to_wg_endpoint: Invalid endpoint format '{}'", endpoint_str);
      return false;
    }

    auto ip = endpoint_str.substr(0, pos);
    auto port = static_cast<uint16_t>(std::stoi(endpoint_str.substr(pos + 1)));

    std::memset(&endpoint, 0, sizeof(wg_endpoint));
    endpoint.addr4.sin_family = AF_INET;
    endpoint.addr4.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &endpoint.addr4.sin_addr) != 1)
    {
      ERROR(channel, "endpoint_to_wg_endpoint: Invalid IP address '{}'", ip);
      return false;
    }
    return true;
  }

  ///
  ///
  auto wg_interface::make_allowedip(const std::string &cidr) -> wg_allowedip *
  {
    auto *aip = new wg_allowedip{};
    size_t slash = cidr.find('/');
    if (slash == std::string::npos)
    {
      return nullptr;
    }

    std::string ip = cidr.substr(0, slash);
    int cidr_val = std::stoi(cidr.substr(slash + 1));

    if (ip.find(':') != std::string::npos)
    {
      // IPv6
      aip->family = AF_INET6;
      aip->cidr = static_cast<uint8_t>(cidr_val);
      inet_pton(AF_INET6, ip.c_str(), &aip->ip6);
    }
    else
    {
      // IPv4
      aip->family = AF_INET;
      aip->cidr = static_cast<uint8_t>(cidr_val);
      inet_pton(AF_INET, ip.c_str(), &aip->ip4);
    }
    aip->next_allowedip = nullptr;
    return aip;
  }

  ///
  ///
  auto wg_interface::convert_allowed_ips(const std::vector<std::string> &allowed_vpn_ips, wg_allowedip *&first, wg_allowedip *&last) -> void
  {
    for (const auto &allowed_ip : allowed_vpn_ips)
    {
      wg_allowedip *aip = make_allowedip(allowed_ip);
      if (!aip)
      {
        continue;
      }
      if (!first)
      {
        first = last = aip;
      }
      else
      {
        last->next_allowedip = aip;
        last = aip;
      }
    }
  }

  ///
  ///
  auto wg_interface::make_unique_wg_device() -> std::unique_ptr<wg_device, wg_device_deleter>
  {
    wg_device *ptr = static_cast<wg_device *>(calloc(1, sizeof(wg_device)));
    if (!ptr)
    {
      throw std::system_error(errno, std::system_category(), "Failed to allocate wg_device");
    }
    return std::unique_ptr<wg_device, wg_device_deleter>(ptr);
  }

} // namespace platform
