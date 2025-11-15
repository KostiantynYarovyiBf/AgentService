
#include "platform/common/tunnel_manager.hpp"

#if defined(__linux__)
  #include "platform/linux/tunnel_manager_impl.hpp"
#elif defined(__APPLE__)
  #include "platform/macos/tunnel_manager_impl.hpp"
#else
  #error "Unsupported platform for tunnel_manager_impl"
#endif

namespace platform
{
///
///
tunnel_manager::tunnel_manager()
  : tunnel_manager_impl_{std::make_unique<tunnel_manager_impl>()}
{
}

///
///
tunnel_manager::~tunnel_manager() = default;

///
///
auto tunnel_manager::create_interface(
  const std::string& interface_name, const std::string& vpn_ip, std::uint16_t listen_port) -> common::error_code
{
  return tunnel_manager_impl_->create_interface(interface_name, vpn_ip, listen_port);
}

///
///
auto tunnel_manager::start_tunnel() -> common::error_code
{
  return tunnel_manager_impl_->start_tunnel();
}

///
///
auto tunnel_manager::stop_tunnel() -> common::error_code
{
  return tunnel_manager_impl_->stop_tunnel();
}

///
///
auto tunnel_manager::add_peer(const common::neighbor_peer_t& peer) -> common::error_code
{
  return tunnel_manager_impl_->add_peer(peer);
}

///
///
auto tunnel_manager::remove_peer(const common::neighbor_peer_t& peer) -> common::error_code
{
  return tunnel_manager_impl_->remove_peer(peer);
}

///
///
auto tunnel_manager::update_peers(const std::vector<common::neighbor_peer_t>& peers) -> common::error_code
{
  return tunnel_manager_impl_->update_peers(peers);
}

///
///
auto tunnel_manager::generate_keys() -> common::error_code
{
  return tunnel_manager_impl_->generate_keys();
}

///
///
auto tunnel_manager::get_public_key() const -> std::string
{
  return tunnel_manager_impl_->get_public_key();
}

///
///
auto tunnel_manager::get_private_key() const -> std::string
{
  return tunnel_manager_impl_->get_private_key();
}

///
///
auto tunnel_manager::is_active() const -> bool
{
  return tunnel_manager_impl_->is_active();
}

///
///
auto tunnel_manager::get_interface_name() const -> std::string
{
  return tunnel_manager_impl_->get_interface_name();
}

///
///
auto tunnel_manager::get_listen_port() const -> std::uint16_t
{
  return tunnel_manager_impl_->get_listen_port();
}

///
///
auto tunnel_manager::get_endpoint(std::uint16_t port) -> std::string
{
  return tunnel_manager_impl_->get_endpoint(port);
}
} // namespace platform
