#include "common/peer_types.hpp"

namespace common
{
    ///
    ///
    peer_t::peer_t(const peer_t &other)
    {
        pub_key = other.pub_key;
        endpoint = other.endpoint;
        vpn_ip = other.vpn_ip;
        registered_at = other.registered_at;
        last_seen = other.last_seen;
        hostname = other.hostname;
    }
    ///
    ///
    peer_t::peer_t(peer_t &&other) noexcept
    {
        pub_key = std::move(other.pub_key);
        endpoint = std::move(other.endpoint);
        vpn_ip = std::move(other.vpn_ip);
        registered_at = std::move(other.registered_at);
        last_seen = std::move(other.last_seen);
        hostname = std::move(other.hostname);
    }
    ///
    ///
    peer_t &peer_t::operator=(const peer_t &other)
    {
        pub_key = other.pub_key;
        endpoint = other.endpoint;
        vpn_ip = other.vpn_ip;
        registered_at = other.registered_at;
        last_seen = other.last_seen;
        hostname = other.hostname;
        return *this;
    }
    ///
    ///
    peer_t &peer_t::operator=(peer_t &&other) noexcept
    {
        pub_key = std::move(other.pub_key);
        endpoint = std::move(other.endpoint);
        vpn_ip = std::move(other.vpn_ip);
        registered_at = std::move(other.registered_at);
        last_seen = std::move(other.last_seen);
        hostname = std::move(other.hostname);
        return *this;
    }

    ///
    ///
    auto peer_t::operator==(const peer_t &other) const -> bool
    {
        return pub_key == other.pub_key &&
               endpoint == other.endpoint &&
               vpn_ip == other.vpn_ip &&
               // registered_at == other.registered_at &&
               // last_seen == other.last_seen &&
               hostname == other.hostname;
    }

    ///
    ///
    neighbor_peer_t::neighbor_peer_t(const neighbor_peer_t &other)
    {
        pub_key = other.pub_key;
        endpoint = other.endpoint;
        allowed_vpn_ips = other.allowed_vpn_ips;
        registered_at = other.registered_at;
        last_seen = other.last_seen;
        hostname = other.hostname;
    }

    ///
    ///
    neighbor_peer_t::neighbor_peer_t(neighbor_peer_t &&other) noexcept
    {
        pub_key = other.pub_key;
        endpoint = other.endpoint;
        allowed_vpn_ips = other.allowed_vpn_ips;
        registered_at = other.registered_at;
        last_seen = other.last_seen;
        hostname = other.hostname;
    }

    ///
    ///
    neighbor_peer_t &neighbor_peer_t::operator=(const neighbor_peer_t &other)
    {
        pub_key = other.pub_key;
        endpoint = other.endpoint;
        allowed_vpn_ips = other.allowed_vpn_ips;
        registered_at = other.registered_at;
        last_seen = other.last_seen;
        hostname = other.hostname;
        return *this;
    }

    ///
    ///
    neighbor_peer_t &neighbor_peer_t::operator=(neighbor_peer_t &&other) noexcept
    {
        pub_key = other.pub_key;
        endpoint = other.endpoint;
        allowed_vpn_ips = other.allowed_vpn_ips;
        registered_at = other.registered_at;
        last_seen = other.last_seen;
        hostname = other.hostname;
        return *this;
    }

    ///
    ///
    auto neighbor_peer_t::operator==(const neighbor_peer_t &other) const -> bool
    {
        return pub_key == other.pub_key &&
               endpoint == other.endpoint &&
               allowed_vpn_ips == other.allowed_vpn_ips &&
               registered_at == other.registered_at &&
               last_seen == other.last_seen &&
               hostname == other.hostname;
    }

} // namespace common
