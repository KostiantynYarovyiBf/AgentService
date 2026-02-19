#pragma once

#include "common/error_codes.hpp"

#include <nlohmann/json.hpp>

#include <string>

///
/// @brief VPN agent configuration structure
///
struct vpn_agent_cfg_v1
{
  std::string ip{"10.0.0.2"};          ///< VPN IP address
  int port{51820};                     ///< Listen port
  std::string hostname{"edge-node-1"}; ///< Agent hostname
  int timer{60};                       ///< Heartbeat interval in seconds
};

using vpn_agent_cfg = vpn_agent_cfg_v1;

///
/// @brief Configuration file manager
///
class config
{
public:
  ///
  /// @brief Constructor
  /// @param path Path to configuration file
  ///
  config(std::string&& path);

  ///
  /// @brief Destructor
  ///
  ~config();

public:
  ///
  /// @brief Get configuration data
  /// @return Reference to configuration structure
  ///
  auto cfg() -> vpn_agent_cfg&;

  ///
  /// @brief Save configuration to file
  /// @return error_code indicating success or failure
  ///
  auto save() -> common::error_code;

private:
  ///
  /// @brief Load configuration from file
  /// @return error_code indicating success or failure
  ///
  auto load() -> common::error_code;

private:
  std::string config_path_;   ///< Path to configuration file
  nlohmann::json data_;       ///< Raw JSON data
  vpn_agent_cfg config_data_; ///< Parsed configuration
};
