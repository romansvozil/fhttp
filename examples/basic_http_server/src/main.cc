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

using server_t = fhttp::server<example_views::views, server_config, example_states::views_shared_state>;

std::unique_ptr<server_t> configure_server(server_config& config) {
    std::unique_ptr<server_t> server = std::make_unique<server_t>(
        config.app_host,
        config.app_port,
        config
    );

    server->set_graceful_shutdown_seconds(4);
    server->set_n_threads(128*2);

    return server;
}

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

    server_config config { };
    config.swagger_json = boost::json::serialize(swagger_json);

    // Initialize the server
    auto server = configure_server(config);

    server->start();
    FHTTP_LOG(INFO) << "Waiting for connections";
    server->wait();
    FHTTP_LOG(INFO) << "Server stopped";

    return 0;
}