#pragma once

#include "config/config.hpp"
#include "common/error_codes.hpp"

#include <atomic>
#include <functional>

class cli
{
public:
    auto run(std::atomic<bool> &running, std::unique_ptr<config> &cfg) -> void;

public:
    std::function<void(std::unique_ptr<config> &cfg, const std::atomic<bool> &running)> register_peer_;
    std::function<void(std::unique_ptr<config> &cfg, const std::atomic<bool> &running)> show_peers_;

    // TODO: add state machine for CLI
};
