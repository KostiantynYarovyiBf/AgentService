#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace common
{

///
/// @brief Represents a peer in the VPN network
///
/// Contains complete peer information including public key, endpoint,
/// VPN IP assignment, and registration metadata. Used for tracking
/// individual peer state in the Control Plane.
///
struct peer_t
{
public: // Constructors
  ///
  /// @brief Default constructor
  ///
  peer_t() = default;

  ///
  /// @brief Destructor
  ///
  ~peer_t() = default;

  ///
  /// @brief Copy constructor
  /// @param other Peer to copy from
  ///
  peer_t(const peer_t&);

  ///
  /// @brief Move constructor
  /// @param other Peer to move from
  ///
  peer_t(peer_t&&) noexcept;

  ///
  /// @brief Copy assignment operator
  /// @param other Peer to copy from
  /// @return Reference to this peer
  ///
  peer_t& operator=(const peer_t&);

  ///
  /// @brief Move assignment operator
  /// @param other Peer to move from
  /// @return Reference to this peer
  ///
  peer_t& operator=(peer_t&&) noexcept;

  ///
  /// @brief Equality comparison operator
  /// @param other Peer to compare with
  /// @return true if peers have the same public key
  ///
  auto operator==(const peer_t& other) const -> bool;

public:                               // Properties
  std::string pub_key;                ///< WireGuard public key
  std::string endpoint;               ///< External endpoint IP address
  std::uint16_t endpoint_port{51820}; ///< External endpoint port
  std::string vpn_ip;                 ///< Assigned VPN IP address (e.g., "10.0.0.1/24")
  std::string registered_at;          ///< ISO 8601 timestamp of registration
  std::string last_seen;              ///< ISO 8601 timestamp of last heartbeat
  std::string hostname;               ///< Host machine name
};

///
/// @brief Represents a neighboring peer for WireGuard configuration
///
/// Contains peer information needed for WireGuard configuration including
/// public key, endpoint, and allowed IPs. Used when configuring local
/// WireGuard interfaces with peer connections.
///
struct neighbor_peer_t
{
public: // Constructors
  ///
  /// @brief Default constructor
  ///
  neighbor_peer_t() = default;

  ///
  /// @brief Destructor
  ///
  ~neighbor_peer_t() = default;

  ///
  /// @brief Copy constructor
  /// @param other Neighbor peer to copy from
  ///
  neighbor_peer_t(const neighbor_peer_t&);

  ///
  /// @brief Move constructor
  /// @param other Neighbor peer to move from
  ///
  neighbor_peer_t(neighbor_peer_t&&) noexcept;

  ///
  /// @brief Copy assignment operator
  /// @param other Neighbor peer to copy from
  /// @return Reference to this neighbor peer
  ///
  neighbor_peer_t& operator=(const neighbor_peer_t&);

  ///
  /// @brief Move assignment operator
  /// @param other Neighbor peer to move from
  /// @return Reference to this neighbor peer
  ///
  neighbor_peer_t& operator=(neighbor_peer_t&&) noexcept;

  ///
  /// @brief Equality comparison operator
  /// @param other Neighbor peer to compare with
  /// @return true if peers have the same public key
  ///
  auto operator==(const neighbor_peer_t& other) const -> bool;

public:                                     // Properties
  std::string pub_key;                      ///< WireGuard public key
  std::string endpoint;                     ///< External endpoint in "IP:PORT" format
  std::vector<std::string> allowed_vpn_ips; ///< List of allowed IPs for WireGuard routing
  std::string registered_at;                ///< ISO 8601 timestamp of registration
  std::string last_seen;                    ///< ISO 8601 timestamp of last heartbeat
  std::string hostname;                     ///< Host machine name
};

///
/// @brief Combined peer information for Control Plane responses
///
/// Contains this peer's own information along with the list of all
/// neighbor peers to connect to. Returned by Control Plane during
/// registration and heartbeat updates.
///
struct self_peer_t
{
  peer_t self;                        ///< This peer's own information
  std::vector<neighbor_peer_t> peers; ///< List of neighbor peers to connect to
  int ttl = 60;                       ///< Time-to-live in seconds for peer list validity
};

} // namespace common
