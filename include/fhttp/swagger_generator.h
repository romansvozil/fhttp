#pragma once

#include "http_server.h"
#include "meta.h"

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

/* === FORWARD DECLERATIONS generate_content_definition === */

template <typename inner_json_t>
inline void generate_content_definition(boost::json::object& schema, const json<inner_json_t>&);

template <typename inner_json_t>
inline std::enable_if_t<has_fields<inner_json_t>::value, void> generate_content_definition(boost::json::object& schema, const inner_json_t&);

/* === IMPLEMENTATIONS generate_content_definition ===*/
template <typename field_t>
inline void generate_content_definition(boost::json::object& schema, const field_t&) requires requires {
    typename field_t::value_type::tuple_type_t;
} {
    generate_content_definition(schema, typename field_t::value_type{});
}

template <typename field_t>
inline std::enable_if_t<!field_t::has_value_type_data_pack, void> generate_content_definition(boost::json::object& schema, const field_t&) requires requires {
    field_t::field_type_name();
} {
    if constexpr (is_specialization<typename field_t::value_type, std::vector>::value) {
        boost::json::object items;
        generate_content_definition(items, typename field_t::value_type::value_type{});
        schema["items"] = items;
        schema["type"] = "array";
    } else {
        schema["type"] = boost::json::string{field_t::field_type_name()};
    }

}

template <typename inner_json_t>
inline std::enable_if_t<has_fields<inner_json_t>::value, void> generate_content_definition(boost::json::object& schema, const inner_json_t&) {
    boost::json::object properties;

    std::apply([&properties](auto&& ... args) {
        ([&] {
            boost::json::object property = {
                {"description", args.description}
            };
            generate_content_definition(property, args);
            properties[args.label] = property;
        } (), ...);
    }, typename inner_json_t::tuple_type_t {});

    schema["properties"] = properties;
    schema["type"] = "object";
}

template <typename inner_json_t>
inline void generate_content_definition(boost::json::object& schema, const json<inner_json_t>&) {
    generate_content_definition(schema, inner_json_t{});
}

// template <typename inner_vector_t>
// inline void generate_content_definition(boost::json::object& schema, const std::vector<inner_vector_t>&) {
//     FHTTP_LOG(INFO) << "Generating array definition";
//     boost::json::object items;
//     generate_content_definition(items, inner_vector_t{});
//     schema["items"] = items;
//     schema["type"] = "array";
// }

template <typename request_t>
inline void generate_content_definition(boost::json::object&, const request_t&) {
    // TODO: define as unknown
    FHTTP_LOG(INFO) << "Generating request body definition for unknown type" << typeid(request_t).name();
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

                    boost::json::object response_body;
                    boost::json::object content;
                    boost::json::object request_type;
                    boost::json::object schema;

                    generate_content_definition(schema, response_t_instance);

                    request_type["schema"] = schema;
                    content["application/json"] = request_type;
                    response_body["content"] = content;
                    responses["200"] = response_body;

                    return responses;
                };

                boost::json::object node;
                node["parameters"] = generate_parameters();
                node["responses"] = generate_responses();

                if (route.method_value == method::post || route.method_value == method::put || route.method_value == method::patch) {
                    const typename request_t::body_type request_t_instance{};
                    
                    boost::json::object request_body;
                    boost::json::object content;
                    boost::json::object request_type;
                    boost::json::object schema;

                    generate_content_definition(schema, request_t_instance);

                    request_type["schema"] = schema;
                    content["application/json"] = request_type;
                    request_body["content"] = content;
                    
                    node["requestBody"] = request_body;
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