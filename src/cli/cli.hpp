#pragma once

#include "config/config.hpp"

#include <atomic>
#include <functional>

class cli
{
public:
    auto run(std::atomic<bool> &running, std::unique_ptr<config> &cfg) -> void;

public:
    std::function<void(std::unique_ptr<config> &cfg)> register_peer_;
    std::function<void(std::unique_ptr<config> &cfg)> show_peers_;

    // TODO: add state machine for CLI
};
