
#pragma once
#include "cli.hpp"
#include "config.hpp"
#include "registration_service.hpp"

#include <thread>
#include <atomic>
#include <memory>

class service
{
public:
    service();
    ~service();

    auto start() -> void;
    auto stop() -> void;

private:
    std::unique_ptr<cli> cli_;
    std::unique_ptr<config> config_;
    std::unique_ptr<registration_service> reg_service_;

    std::thread reg_thread_;
    std::atomic<bool> running_;
};
