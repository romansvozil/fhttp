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

namespace example_views {

struct profile_get_handler: public fhttp::http_handler<profile_get_handler>
    ::with_request_body<fhttp::json<request_json_params>>
    ::with_request_query<query_params>
    ::with_response_body<std::string>
    ::with_description<"Get profile">
 {
    using request_t = typename profile_get_handler::request_t;
    using response_t = typename profile_get_handler::response_t;

    void handle(const request_t&, response_t& response) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        response.body = "Hello world!";
    }
};

struct echo_handler: public fhttp::http_handler<echo_handler> {
    using request_t = typename echo_handler::request_t;
    using response_t = typename echo_handler::response_t;

    void handle(const request_t& request, response_t& response) {
        response.body = request.body;
    }
};

using profile_post_handler = profile_get_handler;
using profile_get_by_id_handler = profile_get_handler;


struct profile_view: public fhttp::view<
    fhttp::route<"/echo", fhttp::method::post, echo_handler>,
    fhttp::route<"/profile", fhttp::method::get, profile_get_handler>,
    fhttp::route<"/profile", fhttp::method::post, profile_post_handler>,
    fhttp::route<"/profile/:id", fhttp::method::get, profile_get_by_id_handler>
> { };

}

struct thread_safe_account_manager {
    std::string get_account_name(const std::string&) {
        return "account name";
    }
};

namespace fhttp {
    template <>
    std::optional<thread_safe_account_manager> create_state(const fhttp::none_config&) {
        FHTTP_LOG(INFO) << "Creating state for thread_safe_account_manager";
        return thread_safe_account_manager {};
    }
}

struct not_thread_safe_friend_manager {
    std::string get_friend_name(const std::string&) {
        return "friend name";
    }
};


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
    
    fhttp::server<example_views::profile_view>
        ::with_global_state<thread_safe_account_manager>
        ::with_thread_local_state<not_thread_safe_friend_manager>
        server { "127.0.0.1", std::to_string(11111) };

    server.start(128*4);

    FHTTP_LOG(INFO) << "Waiting for connections";
    server.wait();
    FHTTP_LOG(INFO) << "Server stopped";

    return 0;
}