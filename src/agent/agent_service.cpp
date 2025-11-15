#include "agent/agent_service.hpp"
#include "logging/logger.hpp"

#include "platform/common/platform_utils.hpp"

#include <chrono>

static constexpr auto channel = "agent_service";

///
///
agent_service::agent_service(const std::string& control_plane_url)
  : running_{false}
  , config_(std::make_unique<config>(platform::platform_utils::get_home_dir() + "/vpn_agent/config.json"))
  , reg_service_(std::make_unique<registration_peers>(control_plane_url))
  , tunnel_manager_(std::make_unique<platform::tunnel_manager>())
{
}

///
///
agent_service::~agent_service()
{
  stop();
}

///
///
auto agent_service::start() -> void
{
  INFO(channel, "Starting agent_service...");

  if(running_)
  {
    WARN(channel, "Service is already running");
    return;
  }

  running_ = true;
  threads_.clear();

  threads_.emplace_back(
    std::jthread(
      [this]()
      {
        INFO(channel, "CLI thread started");
        if(!running_)
        {
          WARN(channel, "Service is stopping, cannot register new peer");
          return;
        }

        if(reg_service_->vpn_up(tunnel_manager_) == common::error_code::success)
        {
          reg_service_->heartbeat(running_, tunnel_manager_);
        }
        else
        {
          ERROR(channel, "Failed to bring VPN up, cleaning up...");
          // TODO: Add proper cleanup logic
        }
        INFO(channel, "CLI thread finished");
      }));

  INFO(channel, "AgentService started successfully with {} background threads", threads_.size());

  while(running_)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

///
///
auto agent_service::stop() -> void
{
  INFO(channel, "Stopping agent_service...");
  if(!running_)
  {
    INFO(channel, "Service is not running, nothing to stop.");
    return;
  }

  running_ = false;

  for(auto& thread : threads_)
  {
    if(thread.joinable())
    {
      INFO(channel, "Waiting for thread to complete...");
      thread.join();
    }
  }

  threads_.clear();
  INFO(channel, "All threads stopped successfully.");
}
