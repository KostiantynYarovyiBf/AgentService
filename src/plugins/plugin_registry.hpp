#pragma once

#include "plugin/plagin_common.hpp"

#include "common/error_codes.hpp"

#include <nlohmann/json.hpp>

namespace plugin
{
class plugin_registry
{
public: // Typedefinitions
  using error_code = common::error_code;

public: // Constructor
  plugin_registry() = default;
  ~plugin_registry() = default;

public: // Methods
  auto resolve(nlohmann::json_abi_v3_11_3::jso specs) -> error_code;

public: // Accessors
  auto plugins() -> const std::vector<plugin_descriptor_t>&;

private: // Helpers
  auto apply_diff_plugin(const std::vector<plugin_descriptor_t>& new_specs) -> error_code;
  auto install(const std::string& id, const std::string& pkg_version, const std::string& pkg) -> error_code;
  auto upgrade(const std::string& id, const std::string& pkg_version, const std::string& pkg) -> error_code;
  auto uninstall(const std::string& id, const std::string& pkg_version) -> error_code;
  auto cleanup_plugins_metada() -> error_code;

  // TODO: validate pkgs (check license, checksum, etc for plugin binary)

private: // Variables
  vector<plugin_descriptor_t> plugins_;
};

} // namespace plugin