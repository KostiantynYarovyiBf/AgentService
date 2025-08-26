#pragma once

#include "common/logging.hpp"
#include "plugin/plagin_common.hpp"

#include <map>
#include <string>

namespace plugin
{
///
/// @brief Supervisor for managing plugin lifecycles (start, stop, restart, monitor)
///
class plugin_supervisor
{
public: // Types
  using error_code = common::error_code;

public: // Constructor
  plugin_supervisor() = default;
  ~plugin_supervisor() = default;

public: // Operations
  auto start(const plugin_descriptor_t& plugin) -> error_code;
  auto start_once(const plugin_descriptor_t& plugin) -> error_code;
  auto stop(const plugin_descriptor_t& plugin) -> error_code;
  auto restart(const plugin_descriptor_t& plugin) -> error_code;
  auto is_alive(const plugin_descriptor_t& plugin) -> error_code;

private:                                                   // Variables
  std::map<std::string, plugin_handel_t> running_plugins_; // Map of plugin ID to process ID
};

} // namespace plugin