#pragma once

#include <string>

namespace platform
{
    class system_info
    {
    public:
        system_info();
        ~system_info();

    public: // System accessors
        static auto get_hostname() -> std::string;

        static auto get_os_version() -> std::string;

        static auto get_host_ip() -> std::string;
    };

}
