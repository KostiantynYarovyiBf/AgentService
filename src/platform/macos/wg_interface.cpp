#include "wg_interface.hpp"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>

namespace platform
{
    ///
    /// Constructor
    ///
    wg_interface::wg_interface() = default;

    ///
    /// Destructor
    ///
    wg_interface::~wg_interface() = default;

    ///
    /// Custom deleter implementation
    ///
    auto wg_interface::wg_device_deleter::operator()(wg_device *ptr) const -> void
    {
        // TODO: Implement proper cleanup when using actual WireGuard API
        // For now, just free the memory if ptr is not null
        if (ptr) {
            free(ptr);
        }
    }

    ///
    ///
    auto wg_interface::setup_wg(std::uint16_t port, const char *wg_name) -> common::error_code
    {
        return common::error_code::success;
    }

    ///
    ///
    auto wg_interface::up_wg_iface(const std::string &ip, const char *wg_name) -> common::error_code
    {
        return common::error_code::success;
    }

    ///
    ///
    auto wg_interface::setup_tunnel(const std::vector<common::neighbor_peer_t> &peers) -> common::error_code
    {
        return common::error_code::success;
    }

    auto wg_interface::pub_key() const -> std::string
    {
        return "";
    }

    auto wg_interface::priv_key() const -> std::string { return ""; }
    auto wg_interface::inface_name() const -> std::string { return ""; }
    auto wg_interface::port_key() const -> std::uint16_t { return 0; }

} // namespace platform