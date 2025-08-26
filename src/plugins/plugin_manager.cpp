#include "plugin/plugin_manager.hpp"

static constexpr auto channel = "plugin_manager";

namespace pipeline
{

///
///
plugin_manager::plugin_manager()
  : plugin_registry_{std::make_unique<plugin_registry>()}
{
}

///
///
auto plugin_manager::init() -> error_code
{
  auto status = error_code::success;
  // TODO:  Setup some default plugins port for communication?
  return status;
}

///
///
auto plugin_manager::apply_pipline_specs(const std::vector<std::string>& plugins) -> error_code
{
  auto status = error_code::success;
  status = plugin_registry_->resolve(new_plugins_list);
  if(status != error_code::success)
  {
    LOG_ERROR(channel, "Failed to resolve plugins list");
    return status;
  }

  return status;
}

} // namespace pipeline