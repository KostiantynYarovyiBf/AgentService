#include "platform/linux/tunnel_manager_impl.hpp"
#include "logging/logger.hpp"
#include "platform/common/platform_utils.hpp"

extern "C"
{
#include <wireguard.h>
}

#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>

#include <boost/asio/ip/address.hpp>

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

static constexpr auto channel = "tunnel_manager_linux";
constexpr auto key_preview_length = 8;

namespace platform
{

struct parsed_cidr_address
{
  int family{};
  uint8_t prefix_len{};
  std::array<uint8_t, sizeof(in6_addr)> bytes{};
  size_t bytes_len{};
};

template<typename uint>
bool parse_int(const std::string& str, int base, uint& n)
{
  try
  {
    size_t pos = 0;
    unsigned long u = stoul(str, &pos, base);
    if(pos != str.length() || u > std::numeric_limits<uint>::max())
      return false;
    n = static_cast<uint>(u);
    return true;
  }
  catch(const std::exception& ex)
  {
    return false;
  }
}

/// @brief Append a netlink route attribute to a netlink message buffer
///
/// Appends a `rtattr` payload to the provided netlink message and updates
/// `nlmsg_len` with proper netlink alignment.
///
/// @param nlh Pointer to target netlink message header
/// @param max_len Total capacity of the message buffer
/// @param type Route attribute type (for example `IFA_LOCAL`)
/// @param data Pointer to attribute payload bytes
/// @param data_len Size of attribute payload in bytes
/// @return true if attribute was appended, false if buffer capacity was exceeded
auto append_rtattr(nlmsghdr* nlh, size_t max_len, uint16_t type, const void* data, size_t data_len) -> bool
{
  auto current_len = NLMSG_ALIGN(nlh->nlmsg_len);
  auto attr_len = RTA_LENGTH(data_len);
  if(current_len + RTA_ALIGN(attr_len) > max_len)
  {
    return false;
  }

  auto* attr = reinterpret_cast<rtattr*>(reinterpret_cast<char*>(nlh) + current_len);
  attr->rta_type = type;
  attr->rta_len = attr_len;
  std::memcpy(RTA_DATA(attr), data, data_len);
  nlh->nlmsg_len = current_len + RTA_ALIGN(attr_len);
  return true;
}

/// @brief Receive netlink acknowledgement for a previously sent request
///
/// Receives netlink responses until `NLMSG_ERROR` is received. An error code of
/// zero indicates ACK success; otherwise this helper sets `errno` from the
/// kernel error and returns failure.
///
/// @param socket_fd Netlink socket file descriptor
/// @return true when ACK is received, false on socket/kernel error
auto receive_netlink_ack(int socket_fd) -> bool
{
  auto response_buffer = std::array<char, 4096>{};
  while(true)
  {
    auto received = recv(socket_fd, response_buffer.data(), response_buffer.size(), 0);
    if(received < 0)
    {
      return false;
    }

    for(auto* nlh = reinterpret_cast<nlmsghdr*>(response_buffer.data());
        NLMSG_OK(nlh, static_cast<unsigned int>(received));
        nlh = NLMSG_NEXT(nlh, received))
    {
      if(nlh->nlmsg_type == NLMSG_ERROR)
      {
        auto* err = reinterpret_cast<nlmsgerr*>(NLMSG_DATA(nlh));
        if(err->error == 0)
        {
          return true;
        }
        errno = -err->error;
        return false;
      }
    }
  }
}

/// @brief Send a netlink request and wait for kernel ACK
///
/// Sends the provided netlink message to the kernel route netlink endpoint and
/// waits for the corresponding ACK via `receive_netlink_ack`.
///
/// @param socket_fd Netlink socket file descriptor
/// @param message Prepared netlink message to send
/// @return true if send and ACK both succeed, false otherwise
auto send_netlink_request_with_ack(int socket_fd, nlmsghdr* message) -> bool
{
  auto kernel_address = sockaddr_nl{};
  kernel_address.nl_family = AF_NETLINK;

  auto io_vector = iovec{};
  io_vector.iov_base = message;
  io_vector.iov_len = message->nlmsg_len;

  auto header = msghdr{};
  header.msg_name = &kernel_address;
  header.msg_namelen = sizeof(kernel_address);
  header.msg_iov = &io_vector;
  header.msg_iovlen = 1;

  if(sendmsg(socket_fd, &header, 0) < 0)
  {
    return false;
  }

  return receive_netlink_ack(socket_fd);
}

/// @brief Parse IPv4/IPv6 address in optional CIDR notation
///
/// Parses an address string such as `10.0.0.2/24`, `fd00::1/64`, or plain host
/// form without prefix. When no prefix is provided, defaults to `/32` for IPv4
/// and `/128` for IPv6.
///
/// @param cidr Address string with optional CIDR suffix
/// @param parsed Output parsed address container
/// @return true if address and prefix are valid, false otherwise
auto parse_cidr_address(const std::string& cidr, parsed_cidr_address& parsed) -> bool
{
  auto slash_pos = cidr.find('/');
  auto address_str = slash_pos == std::string::npos ? cidr : cidr.substr(0, slash_pos);
  auto prefix_len_value = -1;
  if(slash_pos != std::string::npos)
  {
    auto prefix_len_str = cidr.substr(slash_pos + 1);
    prefix_len_value = std::stoi(prefix_len_str);
    if(prefix_len_value < 0 || prefix_len_value > 128)
    {
      return false;
    }
  }

  parsed = parsed_cidr_address{};

  if(inet_pton(AF_INET, address_str.c_str(), parsed.bytes.data()) == 1)
  {
    if(prefix_len_value < 0)
    {
      prefix_len_value = 32;
    }
    if(prefix_len_value > 32)
    {
      return false;
    }
    parsed.prefix_len = static_cast<uint8_t>(prefix_len_value);
    parsed.family = AF_INET;
    parsed.bytes_len = sizeof(in_addr);
    return true;
  }

  if(inet_pton(AF_INET6, address_str.c_str(), parsed.bytes.data()) == 1)
  {
    if(prefix_len_value < 0)
    {
      prefix_len_value = 128;
    }
    parsed.prefix_len = static_cast<uint8_t>(prefix_len_value);
    parsed.family = AF_INET6;
    parsed.bytes_len = sizeof(in6_addr);
    return true;
  }

  return false;
}

/// @brief Assign an IP address to a network interface via netlink
///
/// Builds and sends `RTM_NEWADDR` for the given interface and CIDR address,
/// setting both `IFA_LOCAL` and `IFA_ADDRESS` attributes.
///
/// @param interface_name Target interface name (for example `wg0`)
/// @param cidr_address IPv4/IPv6 address in CIDR form
/// @return true if address assignment is acknowledged by kernel, false otherwise
auto assign_interface_address(const std::string& interface_name, const std::string& cidr_address) -> bool
{
  auto interface_index = if_nametoindex(interface_name.c_str());
  if(interface_index == 0)
  {
    return false;
  }

  auto parsed = parsed_cidr_address{};
  if(!parse_cidr_address(cidr_address, parsed))
  {
    errno = EINVAL;
    return false;
  }

  auto socket_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if(socket_fd < 0)
  {
    return false;
  }

  auto message_buffer = std::array<char, 512>{};
  auto* nlh = reinterpret_cast<nlmsghdr*>(message_buffer.data());
  nlh->nlmsg_len = NLMSG_LENGTH(sizeof(ifaddrmsg));
  nlh->nlmsg_type = RTM_NEWADDR;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_REPLACE;

  auto* ifa = reinterpret_cast<ifaddrmsg*>(NLMSG_DATA(nlh));
  ifa->ifa_family = static_cast<uint8_t>(parsed.family);
  ifa->ifa_prefixlen = parsed.prefix_len;
  ifa->ifa_index = interface_index;

  auto address_data = static_cast<const void*>(parsed.bytes.data());
  auto success = append_rtattr(nlh, message_buffer.size(), IFA_LOCAL, address_data, parsed.bytes_len) &&
                 append_rtattr(nlh, message_buffer.size(), IFA_ADDRESS, address_data, parsed.bytes_len) &&
                 send_netlink_request_with_ack(socket_fd, nlh);

  close(socket_fd);
  return success;
}

/// @brief Set interface administrative state up or down via netlink
///
/// Sends `RTM_NEWLINK` update with `IFF_UP` in `ifi_change`, and applies the
/// requested flag state through `ifi_flags`.
///
/// @param interface_name Target interface name
/// @param is_up true to bring interface up, false to bring it down
/// @return true if state change is acknowledged by kernel, false otherwise
auto set_interface_up_state(const std::string& interface_name, bool is_up) -> bool
{
  auto interface_index = if_nametoindex(interface_name.c_str());
  if(interface_index == 0)
  {
    return false;
  }

  auto socket_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if(socket_fd < 0)
  {
    return false;
  }

  auto message_buffer = std::array<char, 256>{};
  auto* nlh = reinterpret_cast<nlmsghdr*>(message_buffer.data());
  nlh->nlmsg_len = NLMSG_LENGTH(sizeof(ifinfomsg));
  nlh->nlmsg_type = RTM_NEWLINK;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;

  auto* ifi = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(nlh));
  ifi->ifi_family = AF_UNSPEC;
  ifi->ifi_index = static_cast<int>(interface_index);
  ifi->ifi_change = IFF_UP;
  ifi->ifi_flags = is_up ? IFF_UP : 0;

  auto success = send_netlink_request_with_ack(socket_fd, nlh);
  close(socket_fd);
  return success;
}

/// @brief Add or remove a kernel route for a CIDR via a named interface using netlink
///
/// Sends `RTM_NEWROUTE` (add) or `RTM_DELROUTE` (delete) for the given CIDR
/// destination, setting the output interface to `interface_name`. On delete,
/// ENOENT is treated as success (route already absent).
///
/// @param interface_name Output interface name (e.g. `wg0`)
/// @param cidr Destination in CIDR notation (e.g. `10.200.0.0/24`)
/// @param add true to add the route, false to delete it
/// @return true if the netlink operation succeeded, false otherwise
auto manage_interface_route(const std::string& interface_name, const std::string& cidr, bool add) -> bool
{
  auto interface_index = if_nametoindex(interface_name.c_str());
  if(interface_index == 0)
  {
    return false;
  }

  auto parsed = parsed_cidr_address{};
  if(!parse_cidr_address(cidr, parsed))
  {
    errno = EINVAL;
    return false;
  }

  // Zero-out host bits to get the network address
  auto net_bytes = parsed.bytes;
  if(parsed.family == AF_INET)
  {
    uint32_t addr{};
    std::memcpy(&addr, net_bytes.data(), sizeof(uint32_t));
    addr = ntohl(addr);
    if(parsed.prefix_len < 32)
    {
      addr &= ~((1u << (32 - parsed.prefix_len)) - 1u);
    }
    addr = htonl(addr);
    std::memcpy(net_bytes.data(), &addr, sizeof(uint32_t));
  }

  auto socket_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if(socket_fd < 0)
  {
    return false;
  }

  auto message_buffer = std::array<char, 512>{};
  auto* nlh = reinterpret_cast<nlmsghdr*>(message_buffer.data());
  nlh->nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
  nlh->nlmsg_type = add ? RTM_NEWROUTE : RTM_DELROUTE;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
  if(add)
  {
    nlh->nlmsg_flags |= NLM_F_CREATE | NLM_F_REPLACE;
  }

  auto* rtm = reinterpret_cast<rtmsg*>(NLMSG_DATA(nlh));
  rtm->rtm_family = static_cast<uint8_t>(parsed.family);
  rtm->rtm_dst_len = parsed.prefix_len;
  rtm->rtm_src_len = 0;
  rtm->rtm_tos = 0;
  rtm->rtm_table = RT_TABLE_MAIN;
  rtm->rtm_protocol = RTPROT_BOOT;
  rtm->rtm_scope = RT_SCOPE_LINK;
  rtm->rtm_type = RTN_UNICAST;
  rtm->rtm_flags = 0;

  auto oif = static_cast<uint32_t>(interface_index);
  auto success = append_rtattr(nlh, message_buffer.size(), RTA_DST, net_bytes.data(), parsed.bytes_len) &&
                 append_rtattr(nlh, message_buffer.size(), RTA_OIF, &oif, sizeof(oif)) &&
                 send_netlink_request_with_ack(socket_fd, nlh);

  if(!success && !add && errno == ENOENT)
  {
    success = true; // Route already absent — that's fine
  }

  close(socket_fd);
  return success;
}

/// @brief Parse endpoint string "IP:PORT" into WireGuard endpoint structure
///
/// Parses an endpoint string in the format "IP:PORT" and populates a wg_endpoint
/// union with the appropriate sockaddr structure. Supports both IPv4 and IPv6
/// addresses. Uses rfind to handle IPv6 addresses with colons correctly.
///
/// @param endpoint String in format "192.168.1.1:51820" or "[::1]:51820"
/// @param wg_ep Output WireGuard endpoint structure to populate
/// @return true if parsing succeeded, false otherwise
auto parse_endpoint(const std::string& endpoint, wg_endpoint& wg_ep) -> bool
{
  auto pos = endpoint.rfind(':');
  if(pos == std::string::npos || pos == 0 || pos + 1 >= endpoint.length())
  {
    return false;
  }

  auto ip_str = endpoint.substr(0, pos);

  auto port = uint16_t{0};
  if(!parse_int(endpoint.substr(pos + 1), 10, port) || port == 0)
  {
    return false;
  }

  auto is_bracketed_ipv6 = ip_str.size() >= 2 && ip_str.front() == '[' && ip_str.back() == ']';
  auto has_brackets = ip_str.find('[') != std::string::npos || ip_str.find(']') != std::string::npos;
  if(has_brackets && !is_bracketed_ipv6)
  {
    return false;
  }

  if(is_bracketed_ipv6)
  {
    ip_str = ip_str.substr(1, ip_str.size() - 2);
  }

  if(ip_str.empty())
  {
    return false;
  }

  auto ec = boost::system::error_code{};
  auto parsed_address = boost::asio::ip::make_address(ip_str, ec);
  if(ec)
  {
    return false;
  }

  std::memset(&wg_ep, 0, sizeof(wg_ep));

  if(parsed_address.is_v4())
  {
    auto address_bytes = parsed_address.to_v4().to_bytes();
    std::memcpy(&wg_ep.addr4.sin_addr, address_bytes.data(), address_bytes.size());
    wg_ep.addr4.sin_family = AF_INET;
    wg_ep.addr4.sin_port = htons(port);
    return true;
  }

  auto address_bytes = parsed_address.to_v6().to_bytes();
  std::memcpy(&wg_ep.addr6.sin6_addr, address_bytes.data(), address_bytes.size());
  wg_ep.addr6.sin6_family = AF_INET6;
  wg_ep.addr6.sin6_port = htons(port);
  return true;
}

/// @brief Parse allowed IP string "IP/CIDR" into WireGuard allowed IP structure
///
/// Parses an allowed IP string in CIDR notation (e.g., "10.0.0.1/32" or "fd00::/64")
/// and populates a wg_allowedip structure. Supports both IPv4 and IPv6 addresses.
/// The next_allowedip pointer is initialized to nullptr; caller must link multiple
/// allowed IPs if needed.
///
/// @param allowed_ip String in CIDR format, e.g., "10.0.0.1/32"
/// @param wg_aip Output WireGuard allowed IP structure to populate
/// @return true if parsing succeeded, false otherwise
auto parse_allowed_ip(const std::string& allowed_ip, wg_allowedip& wg_aip) -> bool
{
  auto slash_pos = allowed_ip.find('/');
  if(slash_pos == std::string::npos)
  {
    return false;
  }

  auto ip_str = allowed_ip.substr(0, slash_pos);
  auto cidr_str = allowed_ip.substr(slash_pos + 1);
  auto cidr = static_cast<uint8_t>(std::stoi(cidr_str));

  std::memset(&wg_aip, 0, sizeof(wg_aip));
  wg_aip.cidr = cidr;
  wg_aip.next_allowedip = nullptr;

  if(inet_pton(AF_INET, ip_str.c_str(), &wg_aip.ip4) == 1)
  {
    wg_aip.family = AF_INET;
    return true;
  }

  if(inet_pton(AF_INET6, ip_str.c_str(), &wg_aip.ip6) == 1)
  {
    wg_aip.family = AF_INET6;
    return true;
  }

  return false;
}

///
///
tunnel_manager_impl::tunnel_manager_impl()
{
  INFO(channel, "Initializing tunnel manager");

  // Generate keys on initialization
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
  INFO(channel, "Creating WireGuard interface: {} with IP {} on port {}", interface_name, vpn_ip, listen_port);

  interface_name_ = interface_name;
  vpn_ip_ = vpn_ip;
  listen_port_ = listen_port;
  auto existing_device = static_cast<wg_device*>(nullptr);
  if(wg_get_device(&existing_device, interface_name.c_str()) == 0)
  {
    INFO(channel, "Interface {} wg device exist, cleanup it", interface_name);
    wg_free_device(existing_device);
    wg_del_device(interface_name.c_str());
  }

  auto device = wg_device{};

  if(wg_add_device(interface_name.c_str()) < 0)
  {
    if(errno == EEXIST)
    {
      INFO(channel, "Interface {} already exists, reusing", interface_name);
    }
    else
    {
      ERROR(channel, "Failed to create WireGuard interface: {} (errno: {})", interface_name, errno);
      return common::error_code::device_create_failed;
    }
  }

  std::strncpy(device.name, interface_name.c_str(), IFNAMSIZ - 1);
  device.name[IFNAMSIZ - 1] = '\0';
  device.flags = static_cast<wg_device_flags>(WGDEVICE_HAS_PRIVATE_KEY | WGDEVICE_HAS_LISTEN_PORT);
  device.listen_port = listen_port;

  if(wg_key_from_base64(device.private_key, private_key_.c_str()) != 0)
  {
    ERROR(channel, "Failed to decode private key from base64");
    wg_del_device(interface_name.c_str());
    return common::error_code::device_create_failed;
  }

  if(wg_set_device(&device) < 0)
  {
    ERROR(channel, "Failed to configure WireGuard device: {} (errno: {})", interface_name, errno);
    wg_del_device(interface_name.c_str());
    return common::error_code::device_create_failed;
  }

  if(!assign_interface_address(interface_name, vpn_ip))
  {
    WARN(channel, "Failed to assign IP address {} to {} (errno: {})", vpn_ip, interface_name, errno);
  }

  if(!set_interface_up_state(interface_name, true))
  {
    ERROR(channel, "Failed to bring interface up: {} (errno: {})", interface_name, errno);
    wg_del_device(interface_name.c_str());
    return common::error_code::device_create_failed;
  }

  INFO(channel, "WireGuard interface {} created and configured successfully", interface_name);
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::start_tunnel() -> common::error_code
{
  INFO(channel, "Starting WireGuard tunnel on interface: {}", interface_name_);

  if(interface_name_.empty())
  {
    ERROR(channel, "Cannot start tunnel: interface not configured");
    return common::error_code::device_started_failed;
  }

  auto device = static_cast<wg_device*>(nullptr);
  if(wg_get_device(&device, interface_name_.c_str()) < 0)
  {
    ERROR(channel, "Failed to get WireGuard device {}: interface may not exist (errno: {})", interface_name_, errno);
    return common::error_code::device_started_failed;
  }
  wg_free_device(device);

  if(!set_interface_up_state(interface_name_, true))
  {
    ERROR(channel, "Failed to bring interface up: {} (errno: {})", interface_name_, errno);
    return common::error_code::device_started_failed;
  }

  is_active_ = true;
  INFO(channel, "WireGuard tunnel started successfully on {}", interface_name_);
  return common::error_code::success;
}

///
///
auto tunnel_manager_impl::stop_tunnel() -> common::error_code
{
  INFO(channel, "Stopping WireGuard tunnel on interface: {}", interface_name_);

  if(interface_name_.empty())
  {
    WARN(channel, "No interface configured, nothing to stop");
    is_active_ = false;
    return common::error_code::success;
  }

  if(!set_interface_up_state(interface_name_, false))
  {
    WARN(channel, "Failed to bring interface down: {} (errno: {})", interface_name_, errno);
  }

  if(wg_del_device(interface_name_.c_str()) < 0)
  {
    if(errno == ENODEV)
    {
      INFO(channel, "Interface {} already removed", interface_name_);
    }
    else
    {
      ERROR(channel, "Failed to delete WireGuard device {}: (errno: {})", interface_name_, errno);
      return common::error_code::device_stopped_failed;
    }
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

  wg_key private_key = {};
  wg_key public_key = {};

  wg_generate_private_key(private_key);
  wg_generate_public_key(public_key, private_key);

  wg_key_b64_string private_key_b64 = {};
  wg_key_b64_string public_key_b64 = {};

  wg_key_to_base64(private_key_b64, private_key);
  wg_key_to_base64(public_key_b64, public_key);

  private_key_ = private_key_b64;
  public_key_ = public_key_b64;

  INFO(channel, "Generated key pair. Public key: {}...", public_key_.substr(0, key_preview_length));
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

  // Linux-specific settings for better performance and routing
  config_file << "# Linux-specific optimizations\n";
  config_file << "# PostUp = iptables -A FORWARD -i " << interface_name_
              << " -j ACCEPT; iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE\n";
  config_file << "# PostDown = iptables -D FORWARD -i " << interface_name_
              << " -j ACCEPT; iptables -t nat -D POSTROUTING -o eth0 -j MASQUERADE\n";
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

  try
  {
    std::filesystem::permissions(
      config_path,
      std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
      std::filesystem::perm_options::replace);
  }
  catch(const std::exception& e)
  {
    ERROR(channel, "Failed to set secure permissions on configuration file {}: {}", config_path, e.what());
    return common::error_code::file_error;
  }

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
  to_add.clear();
  to_remove.clear();

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

  DEBUG(channel, "Peer diff computed: {} to add, {} to remove", to_add.size(), to_remove.size());
}

///
///
auto tunnel_manager_impl::wg_add_peer_dynamic(const common::neighbor_peer_t& peer) -> common::error_code
{
  INFO(channel, "Adding peer dynamically: {}...", peer.pub_key.substr(0, key_preview_length));

  auto device = wg_device{};
  std::strncpy(device.name, interface_name_.c_str(), IFNAMSIZ - 1);
  device.flags = static_cast<wg_device_flags>(0); // No device-level changes

  auto wg_peer_data = wg_peer{};
  std::memset(&wg_peer_data, 0, sizeof(wg_peer_data));

  if(wg_key_from_base64(wg_peer_data.public_key, peer.pub_key.c_str()) != 0)
  {
    ERROR(channel, "Failed to decode peer public key from base64");
    return common::error_code::peer_add_failed;
  }

  wg_peer_data.flags = static_cast<wg_peer_flags>(
    WGPEER_HAS_PUBLIC_KEY | WGPEER_REPLACE_ALLOWEDIPS | WGPEER_HAS_PERSISTENT_KEEPALIVE_INTERVAL);
  wg_peer_data.persistent_keepalive_interval = 25;

  if(!peer.endpoint.empty())
  {
    if(!parse_endpoint(peer.endpoint, wg_peer_data.endpoint))
    {
      WARN(channel, "Failed to parse endpoint: {}", peer.endpoint);
    }
  }

  auto allowed_ips = std::vector<wg_allowedip>{};
  allowed_ips.reserve(peer.allowed_vpn_ips.size());

  for(const auto& ip : peer.allowed_vpn_ips)
  {
    auto aip = wg_allowedip{};
    if(parse_allowed_ip(ip, aip))
    {
      allowed_ips.push_back(aip);
    }
    else
    {
      WARN(channel, "Failed to parse allowed IP: {}", ip);
    }
  }

  for(size_t i = 0; i < allowed_ips.size(); ++i)
  {
    if(i + 1 < allowed_ips.size())
    {
      allowed_ips[i].next_allowedip = &allowed_ips[i + 1];
    }
  }

  if(!allowed_ips.empty())
  {
    wg_peer_data.first_allowedip = &allowed_ips[0];
    wg_peer_data.last_allowedip = &allowed_ips.back();
  }

  device.first_peer = &wg_peer_data;
  device.last_peer = &wg_peer_data;

  if(wg_set_device(&device) < 0)
  {
    ERROR(channel, "Failed to add peer to WireGuard device (errno: {})", errno);
    return common::error_code::peer_add_failed;
  }

  for(const auto& ip : peer.allowed_vpn_ips)
  {
    if(!manage_interface_route(interface_name_, ip, true))
    {
      WARN(channel, "Failed to add route {} via {} (errno: {})", ip, interface_name_, errno);
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
auto tunnel_manager_impl::wg_remove_peer_dynamic(const common::neighbor_peer_t& peer) -> common::error_code
{
  INFO(channel, "Removing peer dynamically: {}...", peer.pub_key.substr(0, key_preview_length));

  auto device = wg_device{};
  std::strncpy(device.name, interface_name_.c_str(), IFNAMSIZ - 1);
  device.flags = static_cast<wg_device_flags>(0);

  auto wg_peer_data = wg_peer{};
  std::memset(&wg_peer_data, 0, sizeof(wg_peer_data));

  if(wg_key_from_base64(wg_peer_data.public_key, peer.pub_key.c_str()) != 0)
  {
    ERROR(channel, "Failed to decode peer public key from base64");
    return common::error_code::peer_remove_failed;
  }

  wg_peer_data.flags = static_cast<wg_peer_flags>(WGPEER_HAS_PUBLIC_KEY | WGPEER_REMOVE_ME);

  device.first_peer = &wg_peer_data;
  device.last_peer = &wg_peer_data;

  if(wg_set_device(&device) < 0)
  {
    ERROR(channel, "Failed to remove peer from WireGuard device (errno: {})", errno);
    return common::error_code::peer_remove_failed;
  }

  for(const auto& ip : peer.allowed_vpn_ips)
  {
    if(!manage_interface_route(interface_name_, ip, false))
    {
      WARN(channel, "Failed to remove route {} via {} (errno: {})", ip, interface_name_, errno);
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
  return (std::filesystem::path{get_wireguard_config_dir()} / std::filesystem::path{interface_name_ + ".conf"})
    .string();
}

///
///
auto tunnel_manager_impl::get_wireguard_config_dir() const -> std::string
{
  // Standard location (requires root)
  if(access("/etc/wireguard", F_OK) == 0)
  {
    return "/etc/wireguard";
  }

  // User-local fallback
  return platform_utils::get_home_dir() + "/.config/wireguard";
}

} // namespace platform