#include "platform/common/platform_utils.hpp"
#include "logging/logger.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <limits.h> // For POSIX constants
#include <net/if.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

// Use POSIX standard hostname buffer size
#ifndef HOST_NAME_MAX
  #define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#endif

namespace platform
{
static constexpr auto channel = "platform_utils";

constexpr auto kernel_hostname_path_str = "/proc/sys/kernel/hostname";
constexpr auto os_release_path_str = "/etc/os-release";
constexpr auto tmpdir_env_str = "TMPDIR";
constexpr auto pretty_name_regex = "PRETTY_NAME=";

///
/// @brief Helper function to read system files (Linux-specific)
///
/// Reads the entire contents of a file and returns it as a string.
/// Automatically removes trailing newlines. Commonly used for reading
/// /proc and /sys files on Linux.
///
/// @param path Absolute path to the file to read
/// @return File contents as string with trailing newline removed
/// @throws std::runtime_error if the file cannot be opened
///
static auto read_sys_file(const std::string& path) -> std::string
{
  auto file = std::ifstream(path);
  if(!file.is_open())
  {
    throw std::runtime_error("Cannot open file: " + path);
  }

  auto buffer = std::stringstream{};
  buffer << file.rdbuf();
  auto content = buffer.str();

  if(!content.empty() && content.back() == '\n')
  {
    content.pop_back();
  }

  return content;
}

///
///
auto platform_utils::get_hostname() -> std::string
{
  auto hostname_str = std::string{"unknown-linux-host"};

  // POSIX-compliant buffer size
  char hostname[HOST_NAME_MAX + 1];
  if(gethostname(hostname, sizeof(hostname)) == 0)
  {
    hostname_str = std::string(hostname);
  }
  else
  {
    hostname_str = read_sys_file(kernel_hostname_path_str);
  }

  return hostname_str;
}

///
///
auto platform_utils::get_username() -> std::string
{
  auto user_name_str = std::string{"unknown"};
  char buf[LOGIN_NAME_MAX];
  if(getlogin_r(buf, LOGIN_NAME_MAX) == 0)
  {
    user_name_str = std::string(buf);
  }
  else if(const char* env_user = ::getenv("USER"); env_user && *env_user != '\0')
  {
    user_name_str = std::string(env_user);
  }
  return user_name_str;
}

///
///
auto platform_utils::get_os_version() -> std::string
{
  auto os_release = std::ifstream{os_release_path_str};
  auto line = std::string{};
  auto version = std::string{};

  if(os_release.is_open())
  {
    while(std::getline(os_release, line))
    {
      if(line.rfind(pretty_name_regex, 0) == 0)
      {
        // Remove PRETTY_NAME=" and ending "
        version = line.substr(13, line.length() - 14);
        break;
      }
    }
  }

  if(version.empty())
  {
    version = "unknown";
  }
  return version;
}

///
///
auto platform_utils::get_host_ip() -> std::string
{
  auto host_ip = std::string{""};
  struct ifaddrs *ifaddr, *ifa;
  char ip[INET_ADDRSTRLEN] = {0};

  if(getifaddrs(&ifaddr) == -1)
  {
    host_ip = "0.0.0.0";
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
  host_ip = std::string(ip);

  return host_ip;
}

///
///
auto platform_utils::get_home_dir() -> std::string
{
  struct passwd* pw = getpwuid(getuid());
  if(pw != nullptr)
  {
    return std::string(pw->pw_dir);
  }

  return "/home/" + get_username();
}

///
///
auto platform_utils::get_temp_dir() -> std::string
{
  const auto tmp_dir = "/tmp";
  const auto var_tmp_dir = "/var/tmp";
  if(std::filesystem::is_directory(tmp_dir))
  {
    return tmp_dir;
  }

  if(std::filesystem::is_directory(var_tmp_dir))
  {
    return var_tmp_dir;
  }

  throw std::system_error(
    std::make_error_code(std::errc::no_such_file_or_directory),
    std::format("Cannot get temp dir from system by path {} or {}", tmp_dir, var_tmp_dir));
}

///
///
auto platform_utils::create_directory(const std::string& path) -> bool
{
  auto ec = std::error_code{};
  if(std::filesystem::is_directory(path, ec))
  {
    return true;
  }

  ec.clear();
  auto created = std::filesystem::create_directories(path, ec);
  return !ec && (created || std::filesystem::is_directory(path));
}

} // namespace platform
