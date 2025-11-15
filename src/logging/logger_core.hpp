#pragma once

#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

///
/// @brief Logging mode for the application
///
enum class log_mode
{
  console = 0, ///< Console output with colors
  service,     ///< File-based logging for service mode
};

///
/// @brief Global logger singleton
///
class logger
{
public:
  ///
  /// @brief Initialize the global logger
  /// @param mode Logging mode (console or service)
  ///
  static auto init(log_mode mode = log_mode::console) -> void;

  ///
  /// @brief Get the global logger instance
  /// @return Reference to shared logger pointer
  ///
  static auto get() -> std::shared_ptr<spdlog::logger>&;

private:
  static std::shared_ptr<spdlog::logger> s_logger_; ///< Shared logger instance
};
