#include "system_info.hpp"
#include "logging/logger.hpp"

#include <fstream>
#include <format>

#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

// TODO: implement using libnl
// #include <netlink/netlink.h>
// #include <netlink/route/addr.h>
// #include <netlink/route/link.h>
// #include <linux/if.h>

static constexpr auto channel = "system_info";

namespace platform
{

    // TODO: implement using libnl
    // RAII guards for netlink structures
    // struct nl_sock_guard
    // {
    //     nl_sock *s{};
    //     ~nl_sock_guard()
    //     {
    //         if (s)
    //         {
    //             nl_socket_free(s);
    //         }
    //     }
    // };
    // struct rtnl_addr_guard
    // {
    //     rtnl_addr *a{};
    //     ~rtnl_addr_guard()
    //     {
    //         if (a)
    //         {
    //             rtnl_addr_put(a);
    //         }
    //     }
    // };
    // struct rtnl_link_guard
    // {
    //     rtnl_link *l{};
    //     ~rtnl_link_guard()
    //     {
    //         if (l)
    //         {
    //             rtnl_link_put(l);
    //         }
    //     }
    // };

    ///
    ///
    system_info::system_info() = default;

    ///
    ///
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

        if (os_release.is_open())
        {
            while (std::getline(os_release, line))
            {
                if (line.rfind("PRETTY_NAME=", 0) == 0)
                {
                    // Remove PRETTY_NAME=" and ending "
                    version = line.substr(13, line.length() - 14);
                    break;
                }
            }
        }

        if (version.empty())
        {
            version = "unknown";
        }
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

    ///
    ///
    auto system_info::set_interface_ip(const std::string &iface,
                                       const std::string &ip_with_mask) -> common::error_code
    {
        // TODO: implement using libnl
        // nl_sock_guard sock{nl_socket_alloc()};
        // if (!sock.s)
        // {
        //     ERROR(channel, "Failed to allocate netlink socket");
        //     return common::error_code::link_up_failed;
        // }

        // if (nl_connect(sock.s, NETLINK_ROUTE) < 0)
        // {
        //     ERROR(channel, "Failed to connect netlink socket");
        //     return common::error_code::link_up_failed;
        // }

        // int ifindex = rtnl_link_name2i(sock.s, iface.c_str());
        // if (ifindex <= 0)
        // {
        //     ERROR(channel, "Failed to find interface {}: {}", iface, nl_geterror(ifindex));
        //     return common::error_code::link_up_failed;
        // }

        // // Parse CIDR ("10.0.0.2/32" or "10.0.0.2/24")
        // nl_addr *local = nullptr;
        // int rc = nl_addr_parse(ip_with_mask.c_str(), AF_UNSPEC, &local);
        // if (rc < 0)
        // {
        //     ERROR(channel, "Failed to parse IP address {}: {}", ip_with_mask, nl_geterror(rc));
        //     return common::error_code::link_up_failed;
        // }
        // // nl_addr керується самим addr-об'єктом після set_local

        // rtnl_addr_guard addr{rtnl_addr_alloc()};
        // if (!addr.a)
        // {
        //     ERROR(channel, "Failed to allocate address object");
        //     nl_addr_put(local);
        //     return common::error_code::link_up_failed;
        // }

        // rtnl_addr_set_ifindex(addr.a, ifindex);
        // rtnl_addr_set_local(addr.a, local);
        // rtnl_addr_set_family(addr.a, nl_addr_get_family(local));
        // rtnl_addr_set_scope(addr.a, RT_SCOPE_UNIVERSE);

        // rc = rtnl_addr_add(sock.s, addr.a, NLM_F_REPLACE);
        // if (rc < 0 && rc != -NLE_EXIST)
        // {
        //     ERROR(channel, "Failed to set IP address for interface {}: {}", iface, nl_geterror(rc));
        //     return common::error_code::link_up_failed;
        // }

        // rtnl_link_guard link{};
        // rc = rtnl_link_get_kernel(sock.s, ifindex, nullptr, &link.l);
        // if (rc < 0)
        // {
        //     ERROR(channel, "Failed to get link for interface {}: {}", iface, nl_geterror(rc));
        //     return common::error_code::link_up_failed;
        // }

        // unsigned int flags = rtnl_link_get_flags(link.l);
        // if (!(flags & IFF_UP))
        // {
        //     rtnl_link *change = rtnl_link_alloc();
        //     if (!change)
        //     {
        //         ERROR(channel, "Failed to allocate link for modification");
        //         return common::error_code::link_up_failed;
        //     }

        //     rtnl_link_set_flags(change, flags | IFF_UP);
        //     rc = rtnl_link_change(sock.s, link.l, change, 0);
        //     rtnl_link_put(change);
        //     if (rc < 0)
        //     {
        //         ERROR(channel, "Failed to set interface {} up: {}", iface, nl_geterror(rc));
        //         return common::error_code::link_up_failed;
        //     }
        // }

        return common::error_code::success;
    }
}
