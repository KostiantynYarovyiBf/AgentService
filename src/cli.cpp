#include "cli.hpp"
#include "logger.hpp"

#include <iostream>

static constexpr auto channel = "cli";

///
///
auto cli::run(std::atomic<bool> &running) -> void
{
    while (running)
    {
        std::cout << "\n[cli] Menu:\n";
        std::cout << "1 - Action 1\n";
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
            INFO(channel, "Executing Action 1");
            break;
        default:
            WARN(channel, "Unknown option selected: {}", choice);
            break;
        }
    }
}
