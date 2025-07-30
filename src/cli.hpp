#pragma once

#include <atomic>

class cli
{
public:
    auto run(std::atomic<bool> &running) -> void;
};
