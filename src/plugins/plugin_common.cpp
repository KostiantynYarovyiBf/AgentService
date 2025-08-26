#include "plugins/plugin_common.hpp"

namespace plugin
{

auto to_execute_type(const std::string& type_str) -> execute_type
{
  if(type_str == "task")
  {
    return execute_type::task;
  }
  else if(type_str == "service")
  {
    return execute_type::service;
  }

  return execute_type::unknown;
}

} // namespace plugin
