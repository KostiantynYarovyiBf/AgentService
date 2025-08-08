#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

enum class log_mode
{
    console = 0,
    service,
};

class logger
{
public:
    static auto init(log_mode mode = log_mode::console) -> void;
    static auto get() -> std::shared_ptr<spdlog::logger> &;

private:
    static std::shared_ptr<spdlog::logger> s_logger_;
};
