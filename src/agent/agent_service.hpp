
#pragma once
#include "cli/cli.hpp"
#include "config/config.hpp"
#include "registration/registration_peers.hpp"
#include "common/error_codes.hpp"

#include <thread>
#include <atomic>
#include <memory>

class agent_service
{
public:
    agent_service();
    ~agent_service();

    auto start() -> void;
    auto stop() -> void;

    // auto register_cli(std::unique_ptr<cli> &&cli) -> void;

private:
    std::unique_ptr<cli> cli_;
    std::unique_ptr<config> config_;
    std::unique_ptr<registration_peers> reg_service_;

    std::vector<std::jthread> threads_;
    std::atomic<bool> running_;
};
