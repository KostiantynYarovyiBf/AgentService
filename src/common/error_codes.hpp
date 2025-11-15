#pragma once

#include <string>
#include <vector>

namespace common
{
// clang-format off

///
/// @brief Error codes for VPN agent operations
///
/// Common error codes used throughout the application for consistent
/// error handling without exceptions. All functions that can fail should
/// return an error_code value.
///
enum class error_code
{
  not_supported = -1,                 ///< This logic should not be supported in general for this case. For example in some platform only
  success = 0,                        ///< Operation completed successfully
  system_error,                       ///< System-level error (e.g., command execution failed)
  file_error,                         ///< File I/O error (read/write/open failed)
  device_exists,                      ///< Network device already exists
  device_create_failed,               ///< Failed to create network device
  device_started_failed,              ///< Failed to start tunnel
  device_stopped_failed,              ///< Failed to stop tunnel
  peer_add_failed,                    ///< Failed to add peer to tunnel
  peer_remove_failed,                 ///< Failed to remove peer from tunnel
  set_ip_failed,                      ///< Failed to set IP address
  link_up_failed,                     ///< Failed to bring network link up
  allocation_failed,                  ///< Memory or resource allocation failed
  rest_error,                         ///< REST API call failed
  config_save_failed,                 ///< Failed to save configuration
  config_open_failed,                 ///< Failed to open configuration file
  config_load_failed,                 ///< Failed to load/parse configuration
  invalid_endpoint_format,            ///< Endpoint string format is invalid
  invalid_key_format,                 ///< Public or private key format is invalid
  unknown_error                       ///< Unspecified error occurred
};
// clang-format on

} // namespace common
