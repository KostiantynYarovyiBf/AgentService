#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace common
{

    struct peer_t
    {
    public: // Constructors
        peer_t() = default;

        ~peer_t() = default;

        peer_t(const peer_t &);

        peer_t(peer_t &&) noexcept;

        peer_t &operator=(const peer_t &);

        peer_t &operator=(peer_t &&) noexcept;

        auto operator==(const peer_t &other) const -> bool;

    public: // Properties
        std::string pub_key;
        std::string endpoint;
        std::uint16_t endpoint_port{51820};
        std::string vpn_ip;
        std::string registered_at;
        std::string last_seen;
        std::string hostname;
    };

    struct neighbor_peer_t
    {
    public: // Constructors
        neighbor_peer_t() = default;

        ~neighbor_peer_t() = default;

        neighbor_peer_t(const neighbor_peer_t &);

        neighbor_peer_t(neighbor_peer_t &&) noexcept;

        neighbor_peer_t &operator=(const neighbor_peer_t &);

        neighbor_peer_t &operator=(neighbor_peer_t &&) noexcept;

        auto operator==(const neighbor_peer_t &other) const -> bool;

    public: // Properties
        std::string pub_key;
        std::string endpoint;
        std::vector<std::string> allowed_vpn_ips;
        std::string registered_at;
        std::string last_seen;
        std::string hostname;
    };

    struct self_peer_t
    {
        peer_t self;
        std::vector<neighbor_peer_t> peers;
        int ttl = 60;
    };

} // namespace common
