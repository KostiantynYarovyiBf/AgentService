#pragma once

#include "common/error_codes.hpp"
#include "common/peer_types.hpp"

#include <string>
#include <memory>

typedef struct wg_device wg_device;
typedef struct wg_allowedip wg_allowedip;
typedef union wg_endpoint wg_endpoint;

namespace platform
{

  class wg_interface
  {

  private: // Structs
    ///
    /// @brief Custom deleter for wg_device pointers.
    ///
    /// This struct is used as a custom deleter for std::unique_ptr managing
    /// wg_device pointers. When the unique_ptr goes out of scope or is reset,
    /// the deleter will automatically call wg_free_device(ptr) to properly
    /// release all resources associated with the WireGuard device.
    ///
    /// Usage ensures exception safety and prevents memory leaks when handling
    /// wg_device objects allocated with C APIs.
    ///
    struct wg_device_deleter
    {
      auto operator()(wg_device *ptr) const -> void;
    };

  public:
    wg_interface();
    ~wg_interface();

  public: // WireGuard setup methods
    auto setup_wg(std::uint16_t port = 51820, const char *wg_name = "wg0") -> common::error_code;
    auto up_wg_iface(const std::string &ip, const char *wg_name = "wg0") -> common::error_code;
    auto setup_tunnel(const std::vector<common::neighbor_peer_t> &peers) -> common::error_code;

  public: // WireGuard getters
    auto pub_key() const -> std::string;
    auto priv_key() const -> std::string;
    auto inface_name() const -> std::string;
    auto port_key() const -> std::uint16_t;

    // auto get_wg_device() -> std::unique_ptr<wg_device>&;

  private: // Helpers
    auto free_peers() -> void;
    auto endpoint_to_wg_endpoint(const std::string &endpoint_str, wg_endpoint &endpoint) -> bool;
    auto make_allowedip(const std::string &cidr) -> wg_allowedip *;
    auto convert_allowed_ips(const std::vector<std::string> &allowed_vpn_ips, wg_allowedip *&first, wg_allowedip *&last) -> void;

    ///
    /// @brief Factory function to create a unique_ptr-managed wg_device.
    ///
    /// Allocates a new wg_device using calloc and returns a std::unique_ptr
    /// with a custom deleter (wg_device_deleter) to ensure proper cleanup.
    /// Throws std::system_error if allocation fails.
    ///
    /// @return std::unique_ptr<wg_device, wg_device_deleter> owning the allocated wg_device.
    ///
    auto make_unique_wg_device() -> std::unique_ptr<wg_device, wg_device_deleter>;

  private:
    std::unique_ptr<wg_device, wg_device_deleter> wg_device_;
  };

} // namespace platform