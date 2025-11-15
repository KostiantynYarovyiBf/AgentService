#pragma once

#include "common/error_codes.hpp"
#include "common/peer_types.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

///
/// @brief REST client for Control Plane API communication
///
class rest_client
{
public: // Structs
  ///
  /// @brief Heartbeat response from Control Plane
  ///
  struct ping_t
  {
    common::peer_t self;                        ///< Updated peer information
    std::vector<common::neighbor_peer_t> peers; ///< Current peer list
    std::string hash;                           ///< Configuration hash for change detection
    int ttl = 60;                               ///< Time-to-live in seconds
  };

public:
  ///
  /// @brief Constructor
  /// @param base_url Control Plane base URL
  ///
  explicit rest_client(std::string base_url = "http://localhost:4000/_mock/openapi");

  ///
  /// @brief Destructor
  ///
  ~rest_client();

public: // Rest API
  ///
  /// @brief Register peer with Control Plane
  /// @param publicKey WireGuard public key
  /// @param endpoint External endpoint in "IP:PORT" format
  /// @param hostname System hostname
  /// @param out_result Output parameter for registration response
  /// @return error_code indicating success or failure
  ///
  auto register_peer(
    const std::string& publicKey,
    const std::string& endpoint,
    const std::string& hostname,
    common::self_peer_t& out_result) -> common::error_code;

  ///
  /// @brief Send heartbeat ping to Control Plane
  /// @return Optional ping response (nullopt on failure)
  ///
  auto ping() -> std::optional<ping_t>;

public: // Modifiers
  ///
  /// @brief Set Control Plane base URL
  /// @param url New base URL
  ///
  auto base_url(std::string&& url) -> void;

private: // Helpers
  ///
  /// @brief Parse single peer from JSON
  /// @param j JSON object
  /// @return Parsed peer
  ///
  auto parse_agent(const nlohmann::json& j) -> common::peer_t;

  ///
  /// @brief Parse peer list from JSON array
  /// @param jarr JSON array
  /// @return Vector of neighbor peers
  ///
  auto parse_agents(const nlohmann::json& jarr) -> std::vector<common::neighbor_peer_t>;

private:
  std::string base_url_;                    ///< Control Plane base URL
  std::string registered_public_key_;       ///< Public key used to identify this agent on ping
  std::string last_hash_;                   ///< Last known peers hash for conditional ping requests
};