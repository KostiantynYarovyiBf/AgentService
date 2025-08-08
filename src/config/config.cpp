
#include "config/config.hpp"
#include "logging/logger.hpp"

#include <fstream>

static constexpr auto channel = "config";

///
///
config::config(std::string &&path)
    : config_path_(std::move(path))
{
    load();
}

///
///
config::~config()
{
    save();
}

///
///
auto config::cfg() -> vpn_agent_cfg &
{

    return config_data_;
}

///
///
auto config::save() -> void
{
    nlohmann::json j;
    j["ip"] = config_data_.ip;
    j["port"] = config_data_.port;
    j["hostname"] = config_data_.hostname;
    j["timer"] = config_data_.timer;

    std::ofstream f(config_path_);
    if (!f.is_open())
    {
        FATAL(channel, "Failed to open config file for writing: {}", config_path_);
        return;
    }
    f << j.dump(4);
    f.close();
}

///
///
auto config::load() -> void
{
    std::ifstream f(config_path_);
    if (f.is_open())
    {
        nlohmann::json j;
        f >> j;

        if (j.contains("ip"))
            config_data_.ip = j["ip"].get<std::string>();
        if (j.contains("port"))
            config_data_.port = j["port"].get<int>();
        if (j.contains("hostname"))
            config_data_.hostname = j["hostname"].get<std::string>();
        if (j.contains("timer"))
            config_data_.timer = j["timer"].get<int>();
    }
    else
    {
        save();
    }
}
