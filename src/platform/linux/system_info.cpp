#include "system_info.hpp"

#include <fstream>

#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

static constexpr auto channel = "system_info";

namespace platform
{
    system_info::system_info() = default;
    system_info::~system_info() = default;
    ///
    ///
    auto system_info::get_hostname() -> std::string
    {
        char hostname[256] = {0};
        auto hostname_str = std::string{"unknown-host"};
        if (gethostname(hostname, sizeof(hostname)) == 0)
        {
            hostname_str = std::string(hostname);
        }

        return hostname_str;
    }

    ///
    ///
    auto system_info::get_os_version() -> std::string
    {
        std::ifstream os_release("/etc/os-release");
        std::string line, version;

        if (!os_release.is_open())
            return "unknown";

        while (std::getline(os_release, line))
        {
            if (line.rfind("PRETTY_NAME=", 0) == 0)
            {
                // Remove PRETTY_NAME= and any quotes
                version = line.substr(13, line.length() - 14); // Remove PRETTY_NAME=" and ending "
                break;
            }
        }
        if (version.empty())
            return "unknown";
        return version;
    }

    ///
    ///
    auto system_info::get_host_ip() -> std::string
    {
        auto host_ip = std::string{""};
        struct ifaddrs *ifaddr, *ifa;
        char ip[INET_ADDRSTRLEN] = {0};

        if (getifaddrs(&ifaddr) == -1)
        {
            host_ip = "0.0.0.0";
        }

        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
                !(ifa->ifa_flags & IFF_LOOPBACK))
            {
                void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, addr, ip, INET_ADDRSTRLEN);
                break;
            }
        }
        freeifaddrs(ifaddr);
        host_ip = std::string(ip);

        return host_ip;
    }
}
