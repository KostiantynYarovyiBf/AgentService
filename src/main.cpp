#include "logging/logger.hpp"
#include "agent/agent_service.hpp"

int main(int argc, char **argv)
{
    bool serviceMode = false;
    logger::init(serviceMode ? log_mode::service : log_mode::console);

    DEBUG("main", "Starting VPN {}, {}", 22, "testsw");

    agent_service agent_service;
    agent_service.start();
    return 0;
}