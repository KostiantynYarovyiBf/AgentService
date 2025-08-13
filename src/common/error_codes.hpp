#pragma once

#include <string>
#include <vector>

namespace common
{

    enum class error_code
    {
        success = 0,
        device_exists,
        device_create_failed,
        device_configure_failed,
        set_ip_failed,
        link_up_failed,
        allocation_failed,
        rest_error,
        config_save_failed,
        config_open_failed,
        config_load_failed,
        invalid_endpoint_format,
        invalid_pub_key_format,
        unknown_error
    };

} // namespace common
