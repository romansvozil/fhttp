#include "http_server.h"

#include <boost/json.hpp>

namespace fhttp {
namespace swagger {

namespace {

std::string to_lower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str;
}

}

/* Helper template for determining if the type is json */
template <typename T>
struct is_json : std::false_type {};

template <typename T>
struct is_json<json<T>> : std::true_type {};

template <typename inner_json_t>
inline boost::json::object generate_content_definition(const json<inner_json_t>&) {
    /*
    requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              properties:
                username:
                  type: string
    */

    boost::json::object request_body;
    boost::json::object content;
    boost::json::object request_type;
    boost::json::object schema;
    boost::json::object properties;

    std::apply([&properties](auto&& ... args) {
        ([&] {
            properties[args.label] = boost::json::object {
                {"type", args.field_type_name()},
                {"description", args.description}
            };
        } (), ...);
    }, typename inner_json_t::tuple_type_t {});

    // TODO: also implement for arrays
    schema["properties"] = properties;
    schema["type"] = "object";
    request_type["schema"] = schema;
    content["application/json"] = request_type;
    request_body["content"] = content;

    return request_body;
}

template <typename request_t>
inline boost::json::object generate_content_definition(const request_t&) {
    // TODO: define as unknown
    FHTTP_LOG(INFO) << "Generating request body definition for unknown type";
    return {};
}

template <typename view_t>
boost::json::value generateV3(
    const std::string& app_title,
    const std::string& app_version
) {
    boost::json::object swagger;
    swagger["openapi"] = "3.0.0";

    const auto generate_info = [&]() {
        boost::json::object info;
        // TODO: Make this configurable
        info["title"] = app_title;
        info["version"] = app_version;
        return info;
    };

    swagger["info"] = generate_info();

    /// Routes are defined in the view_t [tuple of routes]
    using route_ts = typename view_t::route_ts;

    /// For each route, we need to generate a path object
    boost::json::object paths;

    /// Iterate over the routes
    std::apply([&paths](auto&& ... routes) {
        (..., [&paths](auto&& route) {
            const std::string path = route.path_value;

            const auto create_node = [&] (boost::json::object& path_object) {

                using handler_t = typename std::decay_t<decltype(route)>::handler_type;
                using handler_t_def = handler_type_definition<&handler_t::handle>;
                using request_t = std::decay_t<typename handler_t_def::request_t>;
                using response_t = std::decay_t<typename handler_t_def::response_t>;

                const auto generate_parameters = []() {
                    boost::json::array parameters;
                    return parameters;
                };

                const auto generate_responses = []() {
                    boost::json::object responses;

                    // TODO: somehow get the other possible status codes
                    const typename response_t::body_type response_t_instance{};
                    responses["200"] = generate_content_definition(response_t_instance);

                    return responses;
                };

                boost::json::object node;
                node["parameters"] = generate_parameters();
                node["responses"] = generate_responses();

                if (route.method_value == method::post || route.method_value == method::put || route.method_value == method::patch) {
                    const typename request_t::body_type request_t_instance{};
                    node["requestBody"] =  generate_content_definition(request_t_instance);
                }

                node["summary"] = std::format("{} {}", method_to_string(route.method_value), path);
                node["description"] = handler_t::description;
                path_object[to_lower(method_to_string(route.method_value))] = node;
            };

            if (paths.contains(path)) {
                create_node(paths[path].as_object());
            } else {
                boost::json::object path_obj;
                create_node(path_obj);
                paths[path] = path_obj;
            }
        }(routes));
    }, route_ts{});

    swagger["paths"] = paths;
    return swagger;
}

} // namespace swagger
} // namespace fhttp