#pragma once

#include "common/error_codes.hpp"
#include "plugin/plagin_common.hpp"
#include "plugin/plugin_registry.hpp"

#include <memory>
#include <string>
#include <vector>

namespace plugin
{

class plugin_manager
{
public: // Typedefinitions
  using new_plugin_t = std::pair<supported_plugins, nlohmann::json_abi_v3_11_3::json>;
  using new_plugins_t = std::vector<new_plugin_t>;
  using error_code = common::error_code;

public: // Constructors
  plugin_manager();
  ~plugin_manager() = default;

public: // Actions
  auto init() -> error_code;

  ///
  /// @brief Applies the specified plugin specification by installing, initializing,
  ///        and configuring the given list of plugins in order.
  ///
  /// This function processes the provided list of plugin identifiers, ensuring each
  /// plugin is installed, initialized, and set up with its rules as required by the
  /// plugin specification. It updates the internal plugin list accordingly.
  ///
  /// @param plugins A vector of plugin identifiers (names or IDs) to be applied in the plugin.
  /// @return error_code Error code indicating success or failure of the operation.
  ///
  auto apply_pipline_specs(const std::vector<std::string>& plugins) -> error_code;

private: // Helpers

private: // Variables
  std::unique_ptr<plugin_registry> plugin_registry_;
};

} // namespace plugin
