#include "platform/common/platform_utils.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace platform
{

constexpr auto user_env_str = "USER";
constexpr auto home_env_str = "HOME";
constexpr auto tmpdir_env_str = "TMPDIR";
constexpr auto def_route_ip = "0.0.0.0";

///
///
auto platform_utils::get_hostname() -> std::string
{
  auto hostname_str = std::string{"unknown-macos-host"};

  char hostname[MAXHOSTNAMELEN];
  if(gethostname(hostname, sizeof(hostname)) == 0)
  {
    hostname_str = std::string(hostname);
  }
  else
  {
    size_t size = sizeof(hostname);
    int mib[2] = {CTL_KERN, KERN_HOSTNAME};
    if(sysctl(mib, 2, hostname, &size, nullptr, 0) == 0)
    {
      hostname_str = std::string(hostname);
    }
  }

  return hostname_str;
}

///
///
auto platform_utils::get_username() -> std::string
{
  auto user_name_str = std::string{"unknown"};

  const auto* user = getenv(user_env_str);
  if(user != nullptr)
  {
    user_name_str = std::string(user);
  }
  else
  {
    // Fallback to passwd database
    struct passwd* pw = getpwuid(getuid());
    if(pw != nullptr)
    {
      user_name_str = std::string(pw->pw_name);
    }
  }

  return user_name_str;
}

///
///
auto platform_utils::get_os_version() -> std::string
{
  auto version = std::string{"unknown"};
  char str[256];
  size_t size = sizeof(str);
  int mib[2] = {CTL_KERN, KERN_OSRELEASE};

  if(sysctl(mib, 2, str, &size, nullptr, 0) == 0)
  {
    version = std::string(str);
  }

  return version;
}

///
///
auto platform_utils::get_host_ip() -> std::string
{
  auto host_ip = std::string{""};
  struct ifaddrs* ifaddr = nullptr;
  struct ifaddrs* ifa = nullptr;
  char ip[INET_ADDRSTRLEN] = {0};

  if(getifaddrs(&ifaddr) == -1)
  {
    return def_route_ip;
  }

  for(ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
  {
    if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK))
    {
      void* addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
      inet_ntop(AF_INET, addr, ip, INET_ADDRSTRLEN);
      break;
    }
  }
  freeifaddrs(ifaddr);
  return std::string(ip);
}

///
///
auto platform_utils::get_home_dir() -> std::string
{
  const auto* home = getenv(home_env_str);
  if(home != nullptr)
  {
    return std::string(home);
  }

  struct passwd* pw = getpwuid(getuid());
  if(pw != nullptr)
  {
    return std::string(pw->pw_dir);
  }

  return "/Users/" + get_username();
}

///
///
auto platform_utils::get_temp_dir() -> std::string
{
  const auto* tmp = getenv(tmpdir_env_str);
  if(tmp != nullptr)
  {
    return std::string(tmp);
  }

  // Standard macOS temp directories
  if(access("/tmp", F_OK) == 0)
  {
    return "/tmp";
  }

  return "/var/tmp";
}

///
///
auto platform_utils::create_directory(const std::string& path) -> bool
{
  struct stat st;
  if(stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
  {
    return true;
  }

  auto command = std::string{"mkdir -p \"" + path + "\""};
  return system(command.c_str()) == 0;
}

} // namespace platform
