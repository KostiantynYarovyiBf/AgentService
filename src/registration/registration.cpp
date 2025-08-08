#include "registration/registration.hpp"
#include "logging/logger.hpp"

#include "wg_setup.hpp"
#include <system_info.hpp>

static constexpr auto channel = "registration";

///
///
auto registration::heartbeat(int timer) -> void
{
    // TODO: Implement heartbeat logic
}

///
///
auto registration::vpn_up(std::unique_ptr<config> &config) -> void
{
    DEBUG(channel, "Starting VPN setup...");

    // TODO:
    // Get os.Hostname
    // Get host ip
    // Get public key from wireguard
    // ---------------
    // Get free ip from Control plane
    // Create interface wgN
    // Map wgN
    // ---------------
    // Create interface wgN
    // Set ip (get it from cli)new ip and def port ot wg, Upo this interface

    std::thread([this]()
                {
                    auto hostname = platform::system_info::get_hostname();
                    auto host_ip = platform::system_info::get_host_ip();
                    try{
                        platform::wg_setup::setup_wg();
                        platform::wg_setup::up_wg_iface(host_ip);
                    }
                    catch (const std::system_error &e)
                    {
                        ERROR(channel, "Failed to setup WireGuard: {}", e.what());
                        return;
                    }
                    

                    // TODO: share pub key host name and host ip with control plane
                     

                    DEBUG(channel, "Hostname: {}", hostname);
                    DEBUG(channel, "Host IP: {}", host_ip); })

        .detach();
}
