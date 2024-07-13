#pragma once

#include <string>

#include <fhttp/env.h>

namespace {

std::string get_directory(const std::string& file_path) {
    std::filesystem::path path(file_path);
    return path.parent_path().string();
}

} // anonymous namespace

/// @brief  Example of configuration for the server
struct server_config {
    uint16_t app_port;
    std::string app_host;

    std::string mysql_connection_string { "mysql://localhost:3306" };
    int mysql_timeout { 10 };

    std::string redis_connection_string { "redis://localhost:6379" };
    int redis_timeout { 5 };

    std::string static_files_path { };
    std::string swagger_json;

    server_config()
        : static_files_path(get_directory(__FILE__) + "/www/static/")
    {
        app_port = fhttp::get_env_mandatory<uint16_t>("app_port");
        app_host = fhttp::get_env_mandatory<std::string>("app_host");
        static_files_path = get_directory(__FILE__) + "/www/static/";
    }
};
