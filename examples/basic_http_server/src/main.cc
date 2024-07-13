#include <iostream>
#include <utility>
#include <expected> 

#include <fhttp/http_server.h>
#include <fhttp/logging.h>
#include <fhttp/data/data.h>
#include <fhttp/headers.h>
#include <fhttp/status_codes.h>
#include <fhttp/swagger_generator.h>
#include <fhttp/data/json.h>
#include <fhttp/env.h>

#include "config.h"
#include "models.h"
#include "views.h"
#include "states.h"

namespace {

std::string get_directory(const std::string& file_path) {
    std::filesystem::path path(file_path);
    return path.parent_path().string();
}

} // anonymous namespace

int main() {
    // Initialize the configuration

    /* Generate swagger documentation */
    FHTTP_LOG(INFO) << "Swagger:";
    const auto swagger_json = fhttp::swagger::generateV3<example_views::views>(
        "Example API", "1.0"
    );

    fhttp::datalib::utils::pretty_print(
        FHTTP_LOG(INFO),
        swagger_json
    );

    server_config config {
        "mysql://localhost:3306", 10,
        "redis://localhost:6379", 5,
        get_directory(__FILE__) + "/www/static/",
        boost::json::serialize(swagger_json)
    };

    // Initialize the server
    fhttp::server<example_views::views, server_config, example_states::views_shared_state>
        server { "127.0.0.1", fhttp::get_env_mandatory<uint16_t>("app_port"), config };
    
    server.set_graceful_shutdown_seconds(4);
    server.set_n_threads(128*2);

    server.start();
    FHTTP_LOG(INFO) << "Waiting for connections";
    server.wait();
    FHTTP_LOG(INFO) << "Server stopped";

    return 0;
}