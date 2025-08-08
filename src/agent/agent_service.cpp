#include "agent/agent_service.hpp"
#include "logging/logger.hpp"

static constexpr auto channel = "agent_service";

agent_service::agent_service()
    : running_(false), cli_(std::make_unique<cli>()), config_(std::make_unique<config>("/home/kyaro/vpn_agent/config.json")), reg_service_(std::make_unique<registration>())
{
    cli_->register_peer_ = [this](std::unique_ptr<config> &cfg) -> void
    {
        reg_service_->vpn_up(cfg);
    };

    cli_->show_peers_ = [this](std::unique_ptr<config> &cfg) -> void { /* TODO */ };
}

agent_service::~agent_service()
{
    stop();
}

auto agent_service::start() -> void
{
    INFO(channel, "Starting agent_service...");
    running_ = true;
    cli_->run(running_, config_);

    // reg_thread_ = std::thread([this]()
    //                           { reg_service_->run(running_, config_); });
}

auto agent_service::stop() -> void
{
    if (!running_)
    {
        return;
    }

    INFO(channel, "Stopping agent_service...");
    running_ = false;

    if (reg_thread_.joinable())
    {
        reg_thread_.join();
    }
}
