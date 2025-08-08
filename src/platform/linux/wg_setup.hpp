#pragma once

#include <string>

namespace platform
{
    class wg_setup
    {
    public:
        wg_setup();
        ~wg_setup();

    public: // WireGuard setup methods
        static auto setup_wg(const char *wg_name = "wg0") -> void;
        static auto up_wg_iface(const std::string &ip, const char *wg_name = "wg0") -> void;
    };

}