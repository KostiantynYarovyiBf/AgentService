#include "agent/agent_service.hpp"
#include "logging/logger.hpp"

static constexpr auto channel = "agent_service";

///
///
agent_service::agent_service()
    : running_{false}, cli_(std::make_unique<cli>()), config_(std::make_unique<config>("/home/kyaro/vpn_agent/config.json")), reg_service_(std::make_unique<registration_peers>())
{
    cli_->register_peer_ = [this](std::unique_ptr<config> &cfg, const std::atomic<bool> &running) -> void
    {
        threads_.emplace_back(std::jthread([this, &running]()
                                           {
                                               if (reg_service_->vpn_up() == common::error_code::success)
                                               {
                                                   reg_service_->heartbeat(running);
                                               }
                                               else
                                               {
                                                   // TODO cleanup
                                               } }));
    };

    cli_->show_peers_ = [this](std::unique_ptr<config> &cfg, const std::atomic<bool> &running) -> void
    {
        threads_.emplace_back(std::jthread([this]() { /*TODO */ }));
    };
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
    running_ = true;
    cli_->run(running_, config_);

    // reg_thread_ = std::thread([this]()
    //                           { reg_service_->run(running_, config_); });
}

///
///
auto agent_service::stop() -> void
{
    INFO(channel, "Stopping agent_service...");
    if (!running_)
    {
        INFO(channel, "Service is not running, nothing to stop.");
        return;
    }

    for (auto &thread : threads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}
