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


namespace {

std::string get_directory(const std::string& file_path) {
    std::filesystem::path path(file_path);
    return path.parent_path().string();
}

} // anonymous namespace

namespace example_fields {
    using status =  fhttp::datalib::field<"status", int, "HTTP Status of response">;
    using version = fhttp::datalib::field<"version", std::string, "API Version">;

    using echo = fhttp::datalib::field<"echo", std::string, "Echo Response">;
    using name = fhttp::datalib::field<"name", std::string, "Profile Name">;
    using email = fhttp::datalib::field<"email", std::string, "Profile Email">;

    namespace v1 {

    template <typename T>
    struct response: public fhttp::datalib::data_pack<status, version, T> {
        using super = fhttp::datalib::data_pack<status, version, T>;
        using super::super;

        response() {
            // TODO: what is this syntax???
            this->template set<version>("1.0");
        }

        response(int status_code, const T& data)
            : super(status_code, data)
            , response() {}

    };

    template <typename T>
    using json_response = fhttp::json<response<T>>;

    } // namespace v1

    using response_data = fhttp::datalib::data_pack<status, echo>;
    using profile_data = fhttp::datalib::data_pack<name, email>;

    using profile = fhttp::datalib::field<"profile", profile_data, "User Profile">;

    namespace input {
        using name = fhttp::datalib::field<"name", std::string, "User Name">;
    }

    using profile_request = fhttp::datalib::data_pack<input::name>;
    using echo_request = fhttp::datalib::data_pack<echo>;
}

namespace example_states {

struct fake_redis_manager {
    std::unordered_map<std::string, std::string> cache;

    std::optional<std::string> get(const std::string& key) {
        if (cache.find(key) == cache.end()) {
            return std::nullopt;
        }
        return cache[key];
    }

    void set(const std::string& key, const std::string& value) {
        cache[key] = value;
    }
};

struct fake_sql_manager {
    struct profile {
        std::string name;
        std::string email;
    };

    /// @brief Get profile by name
    /// @param name 
    /// @return profile
    std::optional<profile> get_profile(const std::string& name) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
     
        return profile { name, name + "@example.com" };
    }
};

}

/// @brief  Example of configuration for the server
struct server_config {
    std::string mysql_connection_string;
    int mysql_timeout;

    std::string redis_connection_string;
    int redis_timeout;

    std::string static_files_path;
};

/// @brief Example of creating a state for the server
namespace fhttp {
    template <>
    std::optional<example_states::fake_redis_manager> create_state(const server_config& config) {
        FHTTP_LOG(INFO) << "Creating state for fake_redis_manager";
        
        if (config.redis_connection_string.empty()) {
            return std::nullopt;
        }

        return example_states::fake_redis_manager {};
    }
}

namespace example_views {

using views_shared_state = std::tuple<example_states::fake_redis_manager, example_states::fake_sql_manager>;

/// @brief Create base of our handlers with the server configuration
struct base_handler: public fhttp::http_handler<server_config, views_shared_state> {
    using super = fhttp::http_handler<server_config, views_shared_state>;

    base_handler(const server_config& config, views_shared_state state)
        : super(config, state) {}
};

struct profile_get_handler: public base_handler {
    constexpr static const char* description = "Get profile";

    example_states::fake_sql_manager& sql_manager;

    using request_body_t = fhttp::json<example_fields::profile_request>;
    using response_body_t = example_fields::v1::json_response<example_fields::profile>;

    profile_get_handler(const server_config& config, views_shared_state state)
        : base_handler(config, state)
        , sql_manager(std::get<example_states::fake_sql_manager>(state)) {}

    void handle(
        const fhttp::request<request_body_t>& request,
        fhttp::response<response_body_t>& response
    ) {
        const auto user_name = request.body->get<example_fields::input::name>();
        const auto user = sql_manager.get_profile(user_name);

        if (!user) {
            response.status_code = fhttp::STATUS_CODE_NOT_FOUND;
            return;
        }

        auto& result = response.body->get<example_fields::profile>();

        result.set<example_fields::name>(user->name);
        result.set<example_fields::email>(user->email);

        response.headers[fhttp::HEADER_CONTENT_TYPE] = "application/json";
        response.body->set<example_fields::status>(fhttp::STATUS_CODE_OK);
    }
};

struct echo_handler: public base_handler {
    using request_t = fhttp::request<fhttp::json<example_fields::echo_request>>;

    constexpr static const char* description = "Echo handler";

    echo_handler(const server_config& config, views_shared_state state)
        : base_handler(config, state) {}

    void handle(const request_t& request, fhttp::response<fhttp::json<example_fields::response_data>>& response) {
        response.headers[fhttp::HEADER_CONTENT_TYPE] = "application/json";
        response.body->set<example_fields::echo>(request.body->get<example_fields::echo>());
    }
};

struct hello_handler: public base_handler {
    using request_t = fhttp::request<std::string>;

    constexpr static const char* description = "Echo handler";

    hello_handler(const server_config& config, views_shared_state state)
        : base_handler(config, state) {}

    void handle(const request_t&, fhttp::response<fhttp::json<example_fields::response_data>>& response) {
        response.headers[fhttp::HEADER_CONTENT_TYPE] = "plain/text";
        response.body->set<example_fields::echo>("Hello, World!");
    }
};

struct static_files_handler: public base_handler {
    static_files_handler(const server_config& config, views_shared_state state)
        : base_handler(config, state) {}

    constexpr static const char* description = "Static files handler";

    void handle(const fhttp::request<std::string, query_params>& request, fhttp::response<std::string>& response) {
        auto get_file_content = [](const std::string& path) -> std::string {
            std::ifstream file(path);
            if (!file.is_open()) {
                return "";
            }

            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        };

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
            response.headers[fhttp::HEADER_CONTENT_TYPE] = "text/html";
        }
    }

    bool validate_path(const std::string& path) {
        return path.find("..") == std::string::npos;
    }
};


struct views: public fhttp::view<
    fhttp::route<"/echo", fhttp::method::post, echo_handler>
    , fhttp::route<"/profile", fhttp::method::get, profile_get_handler>
    , fhttp::route<"/static/(?<path>.*)", fhttp::method::get, static_files_handler>
    , fhttp::route<"/hello", fhttp::method::get, hello_handler>
> { };

}

int main() {
    // Initialize the configuration
    server_config config {
        "mysql://localhost:3306", 10,
        "redis://localhost:6379", 5,
        get_directory(__FILE__) + "/www/static/"
    };

    fhttp::server<example_views::views, server_config, example_views::views_shared_state>
        server { "127.0.0.1", 11111, config };
    
    server.set_graceful_shutdown_seconds(4);
    server.set_n_threads(128*2);

    FHTTP_LOG(INFO) << "Swagger:";
    fhttp::datalib::utils::pretty_print(
        FHTTP_LOG(INFO),
        fhttp::swagger::generateV3<example_views::views>(
            "Example API", "1.0"
        )
    );

    server.start();
    FHTTP_LOG(INFO) << "Waiting for connections";
    server.wait();
    FHTTP_LOG(INFO) << "Server stopped";

    return 0;
}