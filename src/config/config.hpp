#pragma once
#include <string>

#include <nlohmann/json.hpp>

struct vpn_agent_cfg_v1
{
    std::string ip{"10.0.0.2"};
    int port{51820};
    std::string hostname{"edge-node-1"};
    int timer{60};
};

using vpn_agent_cfg = vpn_agent_cfg_v1;

class config
{
public:
    config(std::string &&path);
    ~config();

public:
    auto cfg() -> vpn_agent_cfg &;
    auto save() -> void;

private:
    auto load() -> void;

private:
    std::string config_path_;
    nlohmann::json data_;
    vpn_agent_cfg config_data_;
};
