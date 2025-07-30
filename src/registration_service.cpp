#include "registration_service.hpp"
#include "logger.hpp"

#include <thread>
#include <chrono>

static constexpr auto channel = "registration_service";

///
///
auto registration_service::run(std::atomic<bool> &running, std::unique_ptr<config> &config) -> void
{
    while (running)
    {
        DEBUG(channel, "Registration service is running...");
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
