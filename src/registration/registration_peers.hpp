#pragma once

#include "registration/rest_client.hpp"
#include "common/error_codes.hpp"
#include "common/peer_types.hpp"
// #include "config/config.hpp"

#include <wg_interface.hpp>
#include <boost/asio/ip/network_v4.hpp>

#include <atomic>
#include <memory>
#include <vector>

class registration_peers
{
public:
    registration_peers();
    ~registration_peers();

public:
    auto heartbeat(const std::atomic<bool> &) -> void;

    // create interface wgN (do mock)
    // set ip (get it from cli)
    // create public_key in wireguard
    // get hostname (from cli)
    auto vpn_up() -> common::error_code;

private:
    std::unique_ptr<platform::wg_interface> wg_interface_;
    std::unique_ptr<common::self_peer_t> self_peer_;
    std::unique_ptr<rest_client> rest_client_;
};
