#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace plugin
{

enum class execute_type
{
  unknown = -1,
  task = 0,
  service,
};

enum class plugin_state
{
  starting = 0,
  running,
  stopped,
  error,
};

enum class plugin_operation_type
{
  unknown = -1,
  isntall = 0,
  upgrade,
  uninstall,
};

///
///   Static metadata read from local plugin manifest. No runtime fields here.
///   Does not depend on CP payload.
///
struct plugin_descriptor_t
{
  std::string id;
  std::string pkg_version;
  std::string pkg;    //  Path to binary or in-proc symbol
  std::string schema; // Optional JSON Schema for config
  execute_type type;
};

struct plugin_handel_t
{
  std::string id;
  plugin_state state;
  execute_type type;
  std::int32_t pid;
  std::string config_hash;
  // std::unique_ptr<ipc_session_t> ipc_session;
  // std::unique_ptr<data_channel_t> data_channel;
  std::time_t time;
  std::int32_t restart_count;
};

auto to_execute_type(const std::string& type_str) -> execute_type;

} // namespace plugin
