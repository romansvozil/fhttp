#pragma once

#include <string>

/// @brief  Example of configuration for the server
struct server_config {
    std::string mysql_connection_string;
    int mysql_timeout;

    std::string redis_connection_string;
    int redis_timeout;

    std::string static_files_path;
    std::string swagger_json;
};
