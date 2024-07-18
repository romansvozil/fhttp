#pragma once

#include <fhttp/http_server.h>
#include <fhttp/logging.h>
#include <fhttp/data/data.h>
#include <fhttp/headers.h>
#include <fhttp/status_codes.h>
#include <fhttp/data/json.h>

#include "states.h"

namespace example_views {


/// @brief Create base of our handlers with the server configuration
using base_handler = fhttp::http_handler<server_config, example_states::views_shared_state>;

struct profile_post_handler: public base_handler {
    constexpr static const char* description = "Create profile";

    example_states::fake_sql_manager& sql_manager;

    using request_body_t = fhttp::json<example_fields::profile_request>;
    using response_body_t = example_fields::v1::json_response<example_fields::profile>;

    profile_post_handler(const server_config& config, example_states::views_shared_state state)
        : base_handler(config, state)
        , sql_manager(std::get<example_states::fake_sql_manager>(state)) {}

    void handle(
        const fhttp::request<request_body_t>& request,
        fhttp::response<response_body_t>& response
    ) {
        const auto user_name = request.body->get<example_fields::input::name>();
        const auto user = sql_manager.create_profile(user_name);

        if (!user) {
            response.status_code = fhttp::STATUS_CODE_NOT_FOUND;
            response.body->set<example_fields::status>(fhttp::STATUS_CODE_NOT_FOUND);
            return;
        }

        auto& result = response.body->get<example_fields::profile>();

        result.set<example_fields::name>(user->name);
        result.set<example_fields::email>(user->email);

        response.headers[fhttp::HEADER_CONTENT_TYPE] = "application/json";
        response.body->set<example_fields::status>(fhttp::STATUS_CODE_OK);
    }
};

struct profile_post_multiple_handler: public base_handler {
    constexpr static const char* description = "Create profiles";

    example_states::fake_sql_manager& sql_manager;

    using request_body_t = fhttp::json<example_fields::profile_request>;
    using response_body_t = example_fields::v1::json_response<example_fields::profiles>;

    profile_post_multiple_handler(const server_config& config, example_states::views_shared_state state)
        : base_handler(config, state)
        , sql_manager(std::get<example_states::fake_sql_manager>(state)) {}

    void handle(
        const fhttp::request<request_body_t>& request,
        fhttp::response<response_body_t>& response
    ) {
        const auto user_name = request.body->get<example_fields::input::name>();
        const auto user = sql_manager.create_profile(user_name);

        if (!user) {
            response.status_code = fhttp::STATUS_CODE_NOT_FOUND;
            response.body->set<example_fields::status>(fhttp::STATUS_CODE_NOT_FOUND);
            return;
        }

        auto& result = response.body->get<example_fields::profile>();

        result.set<example_fields::name>(user->name);
        result.set<example_fields::email>(user->email);

        response.headers[fhttp::HEADER_CONTENT_TYPE] = "application/json";
        response.body->set<example_fields::status>(fhttp::STATUS_CODE_OK);
    }
};

struct get_all_profiles_handler: public base_handler {
    constexpr static const char* description = "Get all profiles";

    example_states::fake_sql_manager& sql_manager;

    using request_body_t = std::string;
    using response_body_t = example_fields::v1::json_response<example_fields::profiles>;

    get_all_profiles_handler(const server_config& config, example_states::views_shared_state state)
        : base_handler(config, state)
        , sql_manager(std::get<example_states::fake_sql_manager>(state)) {}

    void handle(
        const fhttp::request<request_body_t>&,
        fhttp::response<response_body_t>& response
    ) {
        const auto users = sql_manager.get_all_profiles();

        if (users.empty()) {
            response.status_code = fhttp::STATUS_CODE_NOT_FOUND;
            response.body->set<example_fields::status>(fhttp::STATUS_CODE_NOT_FOUND);
            return;
        }

        auto& result_profiles = response.body->get<example_fields::profiles>();

        for (const auto& profile: users) {
            example_fields::profile_data profile_data;
            profile_data.set<example_fields::name>(profile.name);
            profile_data.set<example_fields::email>(profile.email);
            result_profiles.push_back(profile_data);
        }

        response.headers[fhttp::HEADER_CONTENT_TYPE] = "application/json";
        response.body->set<example_fields::status>(fhttp::STATUS_CODE_OK);
    }
};

struct echo_handler: public base_handler {
    using request_t = fhttp::request<fhttp::json<example_fields::echo_request>>;

    constexpr static const char* description = "Echo handler";

    echo_handler(const server_config& config, example_states::views_shared_state state)
        : base_handler(config, state) {}

    void handle(const request_t& request, fhttp::response<fhttp::json<example_fields::response_data>>& response) {
        response.headers[fhttp::HEADER_CONTENT_TYPE] = "application/json";
        response.body->set<example_fields::echo>(request.body->get<example_fields::echo>());
    }
};

struct hello_handler: public base_handler {
    using request_t = fhttp::request<std::string>;

    constexpr static const char* description = "Echo handler";

    hello_handler(const server_config& config, example_states::views_shared_state state)
        : base_handler(config, state) {}

    void handle(const request_t&, fhttp::response<fhttp::json<example_fields::response_data>>& response) {
        response.headers[fhttp::HEADER_CONTENT_TYPE] = "plain/text";
        response.body->set<example_fields::echo>("Hello, World!");
    }
};

struct static_files_handler: public base_handler {
    static_files_handler(const server_config& config, example_states::views_shared_state state)
        : base_handler(config, state) {}

    constexpr static const char* description = "Static files handler";

    void handle(const fhttp::request<std::string, query_params>& request, fhttp::response<std::string>& response) {
        
        if (!validate_path(request.path)) {
            response.status_code = fhttp::STATUS_CODE_FORBIDDEN;
            return;
        }

        const auto path = config.static_files_path + request.url_matches["path"];
        FHTTP_LOG(INFO) << "Serving file: " << path;

        response.body = get_file_content(path);

        if (response.body.empty()) {
            response.status_code = fhttp::STATUS_CODE_NOT_FOUND;
        } else {
            response.status_code = fhttp::STATUS_CODE_OK;
            response.headers[fhttp::HEADER_CONTENT_TYPE] = "text/html";
        }
    }

private:
    bool validate_path(const std::string& path) {
        return path.find("..") == std::string::npos;
    }

    std::string get_file_content(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
};

struct open_api_json_handler: public base_handler {
    open_api_json_handler(const server_config& config, example_states::views_shared_state state)
        : base_handler(config, state) {}

    constexpr static const char* description = "Open API JSON handler";

    void handle(const fhttp::request<std::string, query_params>&, fhttp::response<std::string>& response) {
        response.body = config.swagger_json;
        response.headers[fhttp::HEADER_CONTENT_TYPE] = "json/application";
    }
};

using root_router = fhttp::router<
    fhttp::route<"/echo",                   fhttp::method::post,    echo_handler>
    , fhttp::route<"/profile",              fhttp::method::post,    profile_post_handler>
    , fhttp::route<"/profile/all",          fhttp::method::post,    get_all_profiles_handler>
    , fhttp::route<"/static/(?<path>.*)",   fhttp::method::get,     static_files_handler>
    , fhttp::route<"/hello",                fhttp::method::get,     hello_handler>
    , fhttp::route<"/openapi.json",         fhttp::method::get,     open_api_json_handler>
>;

} // namespace example_views