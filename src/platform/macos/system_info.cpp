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

        return "";
    }

    ///
    ///
    auto system_info::get_os_version() -> std::string
    {

        return "";
    }

    ///
    ///
    auto system_info::get_host_ip() -> std::string
    {
        return "";
    }

    ///
    ///
    auto system_info::set_interface_ip(const std::string &iface,
                                       const std::string &ip_with_mask) -> common::error_code
    {
        return common::error_code::success;
    }
}
