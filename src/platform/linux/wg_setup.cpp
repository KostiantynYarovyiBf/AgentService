#include "wg_setup.hpp"
#include "logging/logger.hpp"

extern "C"
{
#include <libs/wireguard/wireguard.h>
}

#include <system_error>
#include <cerrno>
#include <format>

static constexpr auto channel = "wg_setup";

namespace platform
{
    ///
    ///
    wg_setup::wg_setup() = default;

    ///
    ///
    wg_setup::~wg_setup() = default;

    ///
    ///
    auto wg_setup::setup_wg(const char *wg_name) -> void
    {
        auto status_code = 0;
        DEBUG(channel, "setup_wg: Initializing WireGuard setup for interface: {}", wg_name);

        wg_device dev;
        memset(&dev, 0, sizeof(dev));
        strncpy(dev.name, wg_name, sizeof(dev.name) - 1);

        wg_key private_key, public_key;
        wg_generate_private_key(private_key);
        wg_generate_public_key(public_key, private_key);

        // Set keys in device struct
        memcpy(dev.private_key, private_key, sizeof(wg_key));
        memcpy(dev.public_key, public_key, sizeof(wg_key));

        dev.listen_port = 51820;

        dev.flags = static_cast<wg_device_flags>(
            static_cast<uint32_t>(dev.flags) | static_cast<uint32_t>(WGDEVICE_HAS_PRIVATE_KEY) | static_cast<uint32_t>(WGDEVICE_HAS_PUBLIC_KEY) | static_cast<uint32_t>(WGDEVICE_HAS_LISTEN_PORT));

        if (if_nametoindex(wg_name) != 0)
        {
            WARN(channel, "setup_wg: WireGuard interface '{}' already exists, skipping creation.", wg_name);
            return;
        }

        status_code = wg_add_device(dev.name);
        if (status_code < 0)
        {
            throw std::system_error(errno, std::system_category(), std::format("setup_wg: Failed to create device wg0. Status code: {}", status_code));
        }

        status_code = wg_set_device(&dev);
        if (status_code < 0)
        {
            throw std::system_error(errno, std::system_category(), std::format("setup_wg: Failed to configure device wg0. Status code: {}", status_code));
        }

        INFO(channel, "setup_wg: wg0 configured successfully");
    }

    ///
    ///
    auto wg_setup::up_wg_iface(const std::string &ip, const char *wg_name) -> void
    {
        auto status_code = 0;
        INFO(channel, "Bringing up WireGuard interface: {}", wg_name);

        std::string ip_cmd = std::format("ip addr add {} dev {}", ip, wg_name);
        status_code = std::system(ip_cmd.c_str());
        if (status_code != 0)
        {
            ERROR(channel, "Failed to set IP address on {} (code: {})", wg_name, status_code);
            return;
        }

        std::string up_cmd = std::format("ip link set {} up", wg_name);
        status_code = std::system(up_cmd.c_str());
        if (status_code != 0)
        {
            ERROR(channel, "Failed to bring {} up (code: {})", wg_name, status_code);
            return;
        }
        INFO(channel, "WireGuard interface [{}] is up with ip [{}]", wg_name, ip);
    }

}
