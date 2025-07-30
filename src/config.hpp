#pragma once
#include <string>

class config
{
public:
    config();
    ~config();

public:
    auto getValue(const std::string &key) -> std::string;
    auto setValue(const std::string &key, const std::string &value) -> void;
};
