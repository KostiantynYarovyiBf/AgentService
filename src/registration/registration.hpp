#pragma once

#include "config/config.hpp"

#include <boost/asio/ip/network_v4.hpp>

#include <atomic>
#include <memory>
#include <vector>

struct peer
{
    std::string device_id;
    std::string public_key;
    std::string endpoint;
    std::vector<std::string> allowed_ips;
};

class registration
{
public:
    auto heartbeat(int timer) -> void;

    // create interface wgN (do mock)
    // set ip (get it from cli)
    // create public_key in wireguard
    // get hostname (from cli)
    auto vpn_up(std::unique_ptr<config> &config) -> void;

private:
    auto get_hostname() -> std::string;
    auto get_host_ip() -> std::string;
    auto setup_wg() -> void;
    auto up_wg_iface(const std::string &ip, const std::string &wg_name) -> void;

private:
    std::vector<peer> peers_;
};
