#pragma once

#include "common/error_codes.hpp"

#include <nlohmann/json.hpp>

#include <string>

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
    auto save() -> common::error_code;

private:
    auto load() -> common::error_code;

private:
    std::string config_path_;
    nlohmann::json data_;
    vpn_agent_cfg config_data_;
};
