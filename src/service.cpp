#include "service.hpp"
#include "logger.hpp"

static constexpr auto channel = "service";

service::service() : running_(false) {}

service::~service()
{
    stop();
}

auto service::start() -> void
{
    running_ = true;
    INFO(channel, "Starting service...");

    reg_thread_ = std::thread([this]()
                              { reg_service_->run(running_, config_); });

    cli_->run(running_);

    stop();
}

auto service::stop() -> void
{
    if (!running_)
    {
        return;
    }

    INFO(channel, "Stopping service...");
    running_ = false;

    if (reg_thread_.joinable())
    {
        reg_thread_.join();
    }
}
