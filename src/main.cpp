#include "agent/agent_service.hpp"
#include "logging/logger.hpp"

#include <memory>
#include <string>

int main(int argc, char** argv)
{
  bool serviceMode = false;
  logger::init(serviceMode ? log_mode::service : log_mode::console);

  auto control_plane_url = std::string{};

  for(auto i = 1; i < argc; ++i)
  {
    auto arg = std::string{argv[i]};
    if(arg == "--control-plane-url")
    {
      if(i + 1 >= argc)
      {
        ERROR("main", "Missing value for --control-plane-url");
        return 1;
      }
      control_plane_url = std::string{argv[++i]};
      continue;
    }

    if(arg == "--help" || arg == "-h")
    {
      INFO("main", "Usage: AgentService [--control-plane-url <url>]");
      return 0;
    }

    WARN("main", "Unknown argument: {}", arg);
  }

  DEBUG("main", "Starting VPN {}, {}", 22, "testsw");

  auto aservice = std::make_unique<agent_service>(control_plane_url);
  aservice->start();
  return 0;
}