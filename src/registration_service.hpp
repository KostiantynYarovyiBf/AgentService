#pragma once

#include "config.hpp"

#include <atomic>
#include <memory>

class registration_service
{
public:
    auto run(std::atomic<bool> &running, std::unique_ptr<config> &config) -> void;
};
