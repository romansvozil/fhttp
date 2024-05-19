#include <iostream>
#include <utility>
#include <expected> 

#include "http_server.h"
#include "logging.h"
#include "datalib.h"


namespace example_fields {

using name = fhttp::datalib::field<"name", std::string>;
using age = fhttp::datalib::field<"age", int>;
using height = fhttp::datalib::field<"height", float>;

struct custom_name_val {
    std::string value;
    
    constexpr custom_name_val() = default;
    constexpr custom_name_val(const std::string& value) : value(value) {}

    constexpr static const char* type_name = "custom_name_val";
};

std::ostream& operator<<(std::ostream& os, const custom_name_val& val) {
    os << val.value;
    return os;
}

using custom_name = fhttp::datalib::field<"custom_name", custom_name_val>;

struct person_entity: public fhttp::datalib::data_pack<name, age, height, custom_name> {
    using base = fhttp::datalib::data_pack<name, age, height, custom_name>;
    using base::base;

    constexpr person_entity() = default;
    constexpr person_entity(const std::string& name, int age, float height) : base(name, age, height, custom_name_val { name + " the Submariner" }) {}

    constexpr int months_age() const {
        return get<age>() * 12;
    }
};

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

}

/// @brief  Example of configuration for the server
struct server_config {
    std::string mysql_connection_string;
    int mysql_timeout;

    std::string redis_connection_string;
    int redis_timeout;
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

/// @brief Create base of our handlers with the server configuration
template <typename parent_handler_t>
struct base_handler: fhttp::http_handler<parent_handler_t, server_config> {};

struct profile_get_handler: public base_handler<profile_get_handler>
    ::with_request_body<fhttp::json<request_json_params>>
    ::with_request_query<query_params>
    ::with_response_body<std::string>
    ::with_description<"Get profile">
    ::with_global_state<example_states::fake_redis_manager>
 {
    void handle(const fhttp::request<fhttp::json<request_json_params>, query_params>&, fhttp::response<std::string>& response) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // auto& account_manager = std::get<example_states::thread_safe_account_manager&>(global_state);
        // FHTTP_LOG(INFO) << "Account count: " << account_manager.create_account();
        // request.
        response.body = "Hello world!";
    }
};

struct echo_handler: public base_handler<echo_handler>
    ::with_request_body<std::string>
    ::with_request_query<query_params>
    ::with_response_body<std::string>
    ::with_global_state<example_states::fake_redis_manager>
    ::with_description<"Get profile">
{
    void handle(const auto& request, auto& response) {
        response.body = request.body;
    }
};


struct profile_view: public fhttp::view<
    fhttp::route<"/echo", fhttp::method::post, echo_handler>,
    fhttp::route<"/profile", fhttp::method::get, profile_get_handler>
> { };

}

int main() {

    fhttp::datalib::field<"name", std::string> name_field { "Roman Svozil" };
    fhttp::datalib::field<"cola", std::string> cola_field { "Kofola is much better tho" };

    FHTTP_LOG(INFO) << "Label: " << name_field.label << " Value: " << name_field.value;
    FHTTP_LOG(INFO) << "Label: " << cola_field.label << " Value: " << cola_field.value;

    FHTTP_LOG(INFO) << "Data pack example: ";

    std::string name { "Namor" };

    example_fields::person_entity person {
        name, 21, 1.8f
    };

    FHTTP_LOG(INFO) << "Name: "        << person.get<example_fields::name>();
    FHTTP_LOG(INFO) << "Custom Name: " << person.get<example_fields::custom_name>().value;
    FHTTP_LOG(INFO) << "Age: "         << person.get<example_fields::age>();
    FHTTP_LOG(INFO) << "Height: "      << person.get<example_fields::height>();
    FHTTP_LOG(INFO) << "Age Months: "  << person.months_age();

    FHTTP_LOG(INFO) << "Instrumenting data pack: ";
    person.instrument();

    FHTTP_LOG(INFO) << "=====JSON=====: ";
    person.json(FHTTP_LOG(INFO));

    FHTTP_LOG(INFO) << "=====HTTP Server=====: ";

    FHTTP_LOG(INFO) << "Running HTTP server...";
    
    // Initialize the configuration
    server_config config {
        "mysql://localhost:3306", 10,
        "redis://localhost:6379", 5
    };

    fhttp::server<example_views::profile_view>
        ::with_global_state<example_states::fake_redis_manager>
        ::with_config<server_config>
        server { "127.0.0.1", std::to_string(11111), config };

    server.start(128*4);

    FHTTP_LOG(INFO) << "Waiting for connections";
    server.wait();
    FHTTP_LOG(INFO) << "Server stopped";

    return 0;
}