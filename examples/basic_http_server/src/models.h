#pragma once

#include <fhttp/data/data.h>

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
            // TODO: somehow fix this ugly thing
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
    using profiles = fhttp::datalib::field<"profiles", std::vector<profile_data>, "User Profiles">;

    namespace input {
        using name = fhttp::datalib::field<"name", std::string, "User Name">;
    }

    using profile_request = fhttp::datalib::data_pack<input::name>;
    using echo_request = fhttp::datalib::data_pack<echo>;
}