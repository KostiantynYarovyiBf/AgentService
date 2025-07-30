
#include "config.hpp"

static constexpr auto channel = "config";

///
///
config::config() = default;

///
///
config::~config() = default;

///
///
auto config::getValue(const std::string &key) -> std::string
{

    return "dummy";
}

///
///
auto config::setValue(const std::string &key, const std::string &value) -> void
{
}
