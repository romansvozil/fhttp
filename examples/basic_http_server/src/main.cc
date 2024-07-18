#include <fhttp/http_server.h>
#include <fhttp/logging.h>
#include <fhttp/swagger_generator.h>

#include "config.h"
#include "models.h"
#include "handlers.h"
#include "states.h"

using server_t = fhttp::server<example_views::root_router, server_config, example_states::views_shared_state>;

std::unique_ptr<server_t> configure_server(server_config& config) {
    auto server = std::make_unique<server_t>(
        config.app_host,
        config.app_port,
        config
    );

    server->set_graceful_shutdown_seconds(config.graceful_shutdown_seconds);
    server->set_n_threads(config.workers);
    server->set_keep_alive_timeout(std::chrono::seconds(3));
    server->set_server_header("Example API");

    return server;
}

int main() {
    /* Generate swagger documentation */
    FHTTP_LOG(INFO) << "Swagger:";
    const auto swagger_json = fhttp::swagger::generateV3<example_views::root_router>(
        "Example API", "1.0"
    );

    fhttp::datalib::utils::pretty_print(
        FHTTP_LOG(INFO),
        swagger_json
    );

    /* Initialize the configuration */
    server_config config { };
    config.swagger_json = boost::json::serialize(swagger_json);

    /* Create the server */
    auto server = configure_server(config);

    /* Start the server */
    server->start();
    FHTTP_LOG(INFO) << "Waiting for connections";
    server->join();
    FHTTP_LOG(INFO) << "Server stopped";

    return 0;
}