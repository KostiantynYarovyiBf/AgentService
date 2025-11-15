
#include "config/config.hpp"
#include "logging/logger.hpp"
#include "platform/common/platform_utils.hpp"

#include <filesystem>
#include <fstream>

static constexpr auto channel = "config";

///
///
config::config(std::string&& path)
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
auto config::cfg() -> vpn_agent_cfg&
{
  return config_data_;
}

///
///
auto config::save() -> common::error_code
{
  auto status = common::error_code::success;
  try
  {
    // Extract directory path from config file path
    std::filesystem::path config_file_path(config_path_);
    std::string config_dir = config_file_path.parent_path().string();

    // Create config directory if it doesn't exist using platform-specific utility
    if(!config_dir.empty() && !platform::platform_utils::create_directory(config_dir))
    {
      ERROR(channel, "Failed to create config directory: {}", config_dir);
      return common::error_code::config_open_failed;
    }

    nlohmann::json j;
    j["ip"] = config_data_.ip;
    j["port"] = config_data_.port;
    j["hostname"] = config_data_.hostname;
    j["timer"] = config_data_.timer;

    std::ofstream f(config_path_);
    if(!f.is_open())
    {
      FATAL(channel, "Failed to open config file for writing: {}", config_path_);
      return common::error_code::config_open_failed;
    }
    f << j.dump(4);
    f.close();
    DEBUG(channel, "Config saved to: {}", config_path_);
  }
  catch(const std::exception& e)
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
    std::filesystem::path config_file_path(config_path_);
    std::string config_dir = config_file_path.parent_path().string();

    if(!config_dir.empty() && !platform::platform_utils::create_directory(config_dir))
    {
      ERROR(channel, "Failed to create config directory: {}", config_dir);
      return common::error_code::config_open_failed;
    }

    std::ifstream f(config_path_);
    if(f.is_open())
    {
      nlohmann::json j;
      f >> j;

      if(j.contains("ip"))
      {
        config_data_.ip = j["ip"].get<std::string>();
      }
      if(j.contains("port"))
      {
        config_data_.port = j["port"].get<int>();
      }
      if(j.contains("hostname"))
      {
        config_data_.hostname = j["hostname"].get<std::string>();
      }
      if(j.contains("timer"))
      {
        config_data_.timer = j["timer"].get<int>();
      }
      f.close();
      DEBUG(channel, "Config loaded from: {}", config_path_);
    }
    else
    {
      INFO(channel, "Config file not found, creating new one at: {}", config_path_);
      status = save();
      if(status == common::error_code::success)
      {
        DEBUG(channel, "New config file created successfully");
      }
    }
  }
  catch(const std::exception& e)
  {
    ERROR(channel, "Failed to load config data: {}", e.what());
    status = common::error_code::config_load_failed;
  }
  return status;
}
