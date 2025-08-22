
#include "config/config.hpp"
#include "logging/logger.hpp"

#include <fstream>

static constexpr auto channel = "config";

///
///
config::config(std::string &&path) : config_path_(std::move(path)) { load(); }

///
///
config::~config() { save(); }

///
///
auto config::cfg() -> vpn_agent_cfg & { return config_data_; }

///
///
auto config::save() -> common::error_code
{
  auto status = common::error_code::success;
  try
  {
    nlohmann::json j;
    j["ip"] = config_data_.ip;
    j["port"] = config_data_.port;
    j["hostname"] = config_data_.hostname;
    j["timer"] = config_data_.timer;

    std::ofstream f(config_path_);
    if (!f.is_open())
    {
      FATAL(channel, "Failed to open config file for writing: {}",
            config_path_);
      return common::error_code::config_open_failed;
    }
    f << j.dump(4);
    f.close();
  }
  catch (const std::exception &e)
  {
    ERROR(channel, "Failed to serialize config data: {}", e.what());
    status = common::error_code::config_save_failed;
  }
  return status;
}

///
///
auto config::load() -> common::error_code
{
  auto status = common::error_code::success;
  try
  {
    std::ifstream f(config_path_);
    if (f.is_open())
    {
      nlohmann::json j;
      f >> j;

      if (j.contains("ip"))
      {
        config_data_.ip = j["ip"].get<std::string>();
      }
      if (j.contains("port"))
      {
        config_data_.port = j["port"].get<int>();
      }
      if (j.contains("hostname"))
      {
        config_data_.hostname = j["hostname"].get<std::string>();
      }
      if (j.contains("timer"))
      {
        config_data_.timer = j["timer"].get<int>();
      }
    }
    else
    {
      status = save();
    }
  }
  catch (const std::exception &e)
  {
    ERROR(channel, "Failed to load config data: {}", e.what());
    status = common::error_code::config_load_failed;
  }
  return status;
}
