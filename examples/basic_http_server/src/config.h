#pragma once

#include <string>

#include <fhttp/env.h>

namespace {

std::string get_parent_directory(const std::string& file_path) {
    std::filesystem::path path(file_path);
    return path.parent_path().parent_path().string();
}

} // anonymous namespace

/// @brief  Example of configuration for the server
struct server_config {
    uint16_t app_port;
    std::string app_host;
    size_t workers { 256 };
    size_t graceful_shutdown_seconds { 1 };

    std::string mysql_connection_string { "mysql://localhost:3306" };
    int mysql_timeout { 10 };

    std::string redis_connection_string { "redis://localhost:6379" };
    int redis_timeout { 5 };

    std::string static_files_path { };
    std::string swagger_json;

    server_config()
        : static_files_path(get_parent_directory(__FILE__) + "/www/static/")
    {
        app_port = fhttp::get_env<uint16_t>("app_port", 11111);
        app_host = fhttp::get_env<std::string>("app_host", "127.0.0.1");
    }
};
