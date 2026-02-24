#include "registration/rest_client.hpp"
#include "logging/logger.hpp"

#include <cpr/cpr.h>

using json = nlohmann::json;

static constexpr auto channel = "rest_client";

///
///
rest_client::rest_client(std::string base_url)
  : base_url_(std::move(base_url))
{
}

///
///
rest_client::~rest_client() = default;

///
///
auto rest_client::register_peer(
  const std::string& publicKey,
  const std::string& endpoint,
  const std::string& hostname,
  common::self_peer_t& out_result) -> common::error_code

{
  DEBUG(channel, "register new peer: publicKey {}, endpoint {}, hostname {}", publicKey, endpoint, hostname);
  auto payload = json::object({
    {"publicKey", publicKey},
    { "endpoint",  endpoint},
    { "hostname",  hostname}
  });

  try
  {
    auto res = cpr::Post(
      cpr::Url{
        base_url_ + "/register"
    },
      cpr::Header{{"Content-Type", "application/json"}},
      cpr::Body{payload.dump()});

    if(res.status_code != 200)
    {
      ERROR(channel, "registerPeer failed: HTTP {} body={}", res.status_code, res.text);
      return common::error_code::rest_error;
    }

    auto body = json::parse(res.text);
    if(body.contains("self") && body.contains("peers") && body.contains("ttl"))
    {
      out_result.self = parse_agent(body.at("self"));
      out_result.peers = parse_agents(body.at("peers"));
      out_result.ttl = body.at("ttl").get<int>();
      registered_public_key_ = publicKey;
      if(body.contains("hash"))
      {
        last_hash_ = body.at("hash").get<std::string>();
      }
      else
      {
        last_hash_.clear();
      }
    }
    else
    {
      ERROR(channel, "registerPeer response is missing required fields: {}", body.dump());
      return common::error_code::rest_error;
    }

    return common::error_code::success;
  }
  catch(const std::exception& e)
  {
    ERROR(channel, "registerPeer exception: {}", e.what());
    return common::error_code::rest_error;
  }
}

///
///
auto rest_client::ping() -> std::optional<rest_client::ping_t>
{
  cpr::Header hdrs;
  ping_t out;

  if(registered_public_key_.empty())
  {
    ERROR(channel, "ping skipped: agent is not registered yet (missing public key)");
    return std::nullopt;
  }

  hdrs.insert({"X-Agent-PubKey", registered_public_key_});
  if(!last_hash_.empty())
  {
    hdrs.insert({"X-Peers-Hash", last_hash_});
  }

  auto res = cpr::Get(cpr::Url{base_url_ + "/ping"}, cpr::Header{hdrs});

  if(res.status_code == 304)
  {
    DEBUG(channel, "ping not modified for key {}", registered_public_key_);
    return std::nullopt;
  }

  if(res.status_code == 404)
  {
    WARN(channel, "ping: agent not registered on server (evicted?), clearing registration key");
    registered_public_key_.clear();
    return std::nullopt;
  }

  if(res.status_code != 200)
  {
    throw std::runtime_error("ping failed: HTTP " + std::to_string(res.status_code) + " body=" + res.text);
  }

  auto body = json::parse(res.text);
  if(!body.is_object())
  {
    throw std::runtime_error("ping response is not an object: " + res.text);
  }
  if(body.contains("self"))
  {
    out.self = parse_agent(body.at("self"));
  }
  if(body.contains("peers"))
  {
    out.peers = parse_agents(body.at("peers"));
  }
  if(body.contains("hash"))
  {
    out.hash = body.at("hash").get<std::string>();
    last_hash_ = out.hash;
  }
  else
  {
    last_hash_.clear();
  }
  if(body.contains("ttl"))
  {
    out.ttl = body.at("ttl").get<int>();
  }

  return out;
}

///
///
auto rest_client::is_registered() const -> bool
{
  return !registered_public_key_.empty();
}

///
///
auto rest_client::base_url(std::string&& url) -> void
{
  base_url_ = std::move(url);
}

///
///
auto rest_client::parse_agent(const json& j) -> common::peer_t
{
  common::peer_t a;
  if(
    j.contains("publicKey") && j.contains("endpoint") && j.contains("VpnIp") && j.contains("registeredAt") &&
    j.contains("lastSeen") && j.contains("hostname"))
  {
    a.pub_key = j.at("publicKey").get<std::string>();
    a.endpoint = j.at("endpoint").get<std::string>();
    a.vpn_ip = j.at("VpnIp").get<std::string>();
    a.registered_at = j.at("registeredAt").get<std::string>();
    a.last_seen = j.at("lastSeen").get<std::string>();
    a.hostname = j.at("hostname").get<std::string>();
  }
  else
  {
    ERROR(channel, "parse_agent response is missing required fields: {}", j.dump());
    throw std::runtime_error("parse_agent response is missing required fields");
  }
  return a;
}

///
///
auto rest_client::parse_agents(const json& jarr) -> std::vector<common::neighbor_peer_t>
{
  std::vector<common::neighbor_peer_t> out;
  if(!jarr.is_array())
  {
    return out;
  }
  out.reserve(jarr.size());
  for(const auto& item : jarr)
  {
    common::neighbor_peer_t a;
    if(item.contains("publicKey"))
    {
      a.pub_key = item.at("publicKey").get<std::string>();
    }
    if(item.contains("endpoint"))
    {
      a.endpoint = item.at("endpoint").get<std::string>();
    }
    if(item.contains("allowedVpnIps"))
    {
      for(const auto& ip : item.at("allowedVpnIps"))
      {
        a.allowed_vpn_ips.emplace_back(ip.get<std::string>());
      }
    }
    if(item.contains("registeredAt"))
    {
      a.registered_at = item.at("registeredAt").get<std::string>();
    }
    if(item.contains("lastSeen"))
    {
      a.last_seen = item.at("lastSeen").get<std::string>();
    }
    if(item.contains("hostname"))
    {
      a.hostname = item.at("hostname").get<std::string>();
    }
    out.emplace_back(std::move(a));
  }
  return out;
}
