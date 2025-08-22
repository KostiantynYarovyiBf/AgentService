#pragma once

#include "common/error_codes.hpp"
#include "common/peer_types.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

class rest_client
{

public: // Structs
  struct ping_t
  {
    common::peer_t self;
    std::vector<common::neighbor_peer_t> peers;
    std::string hash; // new config hash
    int ttl = 60;
  };

public:
  explicit rest_client(std::string base_url = "http://localhost:4000/_mock/openapi");

  ~rest_client();

public: // Rest API
  auto register_peer(const std::string &publicKey, const std::string &endpoint,
                     const std::string &hostname) -> common::self_peer_t;

  auto ping() -> std::optional<ping_t>;

public: // Modifiers
  auto base_url(std::string &&url) -> void;

private: // Helpers
  auto parse_agent(const nlohmann::json &j) -> common::peer_t;
  auto parse_agents(const nlohmann::json &jarr) -> std::vector<common::neighbor_peer_t>;

private:
  std::string base_url_;
};