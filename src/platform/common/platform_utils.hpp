#pragma once

#include "common/error_codes.hpp"

#include <cstdint>
#include <string>

namespace platform
{

///
/// @brief Generic OS utility functions for cross-platform operations
///
/// Provides static utility methods for common OS operations like
/// file paths, system information, and command execution.
/// Platform-specific implementations are selected by CMake at build time.
///
class platform_utils
{
public: // Constructors
        ///
        /// Static utility class
        ///
  platform_utils() = delete;

public: // System information
  ///
  /// @brief Get the system hostname
  /// @return Hostname as string
  ///
  static auto get_hostname() -> std::string;

  ///
  /// @brief Get current username
  /// @return Username as string
  ///
  static auto get_username() -> std::string;

  ///
  /// @brief Get os version information
  /// @return Os version
  ///
  static auto get_os_version() -> std::string;

  ///
  /// @brief Get host IP address (external/public IP)
  /// @return IP address as string
  ///
  static auto get_host_ip() -> std::string;

public: // File system paths
  ///
  /// @brief Get the user home directory
  /// @return Path to user home directory
  ///
  static auto get_home_dir() -> std::string;

  ///
  /// @brief Get the temporary directory path
  /// @return Path to temp directory
  ///
  static auto get_temp_dir() -> std::string;

  ///
  /// @brief Create directory if it doesn't exist (mkdir -p behavior)
  /// @param path Directory path to create
  /// @return true if directory exists or was created successfully
  ///
  static auto create_directory(const std::string& path) -> bool;
};

} // namespace platform
