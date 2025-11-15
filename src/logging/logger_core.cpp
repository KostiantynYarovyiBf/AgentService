#include "logging/logger_core.hpp"

std::shared_ptr<spdlog::logger> logger::s_logger_;

///
///
auto logger::init(log_mode mode) -> void
{
  try
  {
    if(mode == log_mode::console)
    {
      auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

      const auto* home_ptr = std::getenv("HOME");
      std::string log_path = home_ptr != nullptr ? std::string(std::getenv("HOME")) + "/agent.log" : "/tmp/agent.log";
      auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);
      s_logger_ = std::make_shared<spdlog::logger>("Agentlogger", spdlog::sinks_init_list{console_sink, file_sink});
    }
    else
    {
      auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/var/log/agent_service.log", true);
      s_logger_ = std::make_shared<spdlog::logger>("Agentlogger", spdlog::sinks_init_list{file_sink});
    }

    s_logger_->set_level(spdlog::level::debug);
    s_logger_->flush_on(spdlog::level::info);
    spdlog::set_default_logger(s_logger_);
  }
  catch(const spdlog::spdlog_ex& ex)
  {
    printf("Log init failed: %s\n", ex.what());
  }
}

///
///
auto logger::get() -> std::shared_ptr<spdlog::logger>&
{
  if(!s_logger_)
  {
    init(log_mode::console);
  }
  return s_logger_;
}
