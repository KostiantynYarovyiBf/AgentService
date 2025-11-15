#pragma once

#include "logging/logger_core.hpp"

#include <format>
#include <string>

///
///
#define LOG(CHANNEL, LEVEL, FMT_STR, ...)                                                                              \
  {                                                                                                                    \
    auto logstring = std::string{};                                                                                    \
    try                                                                                                                \
    {                                                                                                                  \
      logstring = std::format("[{}] ", CHANNEL) + std::format(FMT_STR, ##__VA_ARGS__);                                 \
    }                                                                                                                  \
    catch(const std::format_error& exception)                                                                          \
    {                                                                                                                  \
      logstring = std::format("INTERNAL_STD_STR format error: {}.", exception.what());                                 \
    }                                                                                                                  \
    auto& logger_instance = ::logger::get();                                                                           \
    if(logger_instance)                                                                                                \
    {                                                                                                                  \
      logger_instance->log(LEVEL, std::move(logstring));                                                               \
    }                                                                                                                  \
  }

#define DEBUG(CHANNEL, FMT_STR, ...) LOG((CHANNEL), spdlog::level::debug, FMT_STR __VA_OPT__(, ) __VA_ARGS__);

#define INFO(CHANNEL, FMT_STR, ...) LOG((CHANNEL), spdlog::level::info, FMT_STR __VA_OPT__(, ) __VA_ARGS__);

#define WARN(CHANNEL, FMT_STR, ...) LOG((CHANNEL), spdlog::level::warn, FMT_STR __VA_OPT__(, ) __VA_ARGS__);

#define ERROR(CHANNEL, FMT_STR, ...) LOG((CHANNEL), spdlog::level::err, FMT_STR __VA_OPT__(, ) __VA_ARGS__);

#define FATAL(CHANNEL, FMT_STR, ...) LOG((CHANNEL), spdlog::level::critical, FMT_STR __VA_OPT__(, ) __VA_ARGS__);
