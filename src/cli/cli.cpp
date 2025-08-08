#include "cli/cli.hpp"
#include "logging/logger.hpp"

#include <iostream>

static constexpr auto channel = "cli";

///
///
auto cli::run(std::atomic<bool> &running, std::unique_ptr<config> &cfg) -> void
{
    while (running)
    {
        std::cout << "\n[cli] Menu:\n";
        std::cout << "1 - wpn up\n";
        std::cout << "0 - Exit\n";
        std::cout << "Choice: ";

        int choice;
        std::cin >> choice;

        switch (choice)
        {
        case 0:
            FATAL(channel, "Exiting CLI");
            std::exit(0);
            break;
        case 1:
            INFO(channel, "VPN up");
            if (register_peer_)
            {
                register_peer_(cfg);
            }
            else
            {
                WARN(channel, "No peer registration function set");
            }
            break;
        default:
            WARN(channel, "Unknown option selected: {}", choice);
            break;
        }
    }
}
