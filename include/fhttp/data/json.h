#pragma once

#include <boost/json.hpp>

#include "../meta.h"

#include <optional>

namespace fhttp {
namespace datalib {

namespace utils {

void pretty_print(std::ostream& os, boost::json::value const& jv, std::string* indent);

} // namespace utils

namespace serialization {

template <typename content_t>
std::enable_if_t<!has_fields<content_t>::value, std::optional<boost::json::value>> to_json(const content_t& content) {
    boost::json::value val;
    val = content;
    return val;
}

template <typename content_t>
std::optional<boost::json::value> to_json(const std::vector<content_t>& content) {
    boost::json::array arr {content.begin(), content.end()};

    return arr;
}

template <typename content_t>
std::optional<boost::json::value> to_json(const std::unordered_map<std::string, content_t>& content) {
    boost::json::object obj_map;

    for (const auto& [key, value] : content) {
        obj_map[key] = value;
    }

    return obj_map;
}

template <typename content_t>
std::enable_if_t<has_fields<content_t>::value, std::optional<boost::json::value>> to_json(const content_t& data_pack_content) {
    boost::json::object obj;

    std::apply([&obj](auto&& ... args) {
        ([&] {
            if (const auto serialized = to_json(args.value); serialized) {
                obj[args.label] = *serialized;
            }
        } (), ...);
    }, data_pack_content.fields);

    return obj;
}

} // namespace serialization

#if 0 // TODO: Finish this
namespace deserialization {

template <typename content_t>
std::enable_if_t<!has_fields<content_t>::value, std::optional<content_t>> from_json(const boost::json::value& json_value) {
    // TODO: here we will probably need to check if the json_value is of the same 
    // type as content_t, and maybe create overloads for each json primitive
    return json_value.as<content_t>();
}

template <typename content_t>
std::optional<std::vector<content_t>> from_json(const boost::json::value& json_value) {
    boost::json::array arr = json_value.as_array();
    std::vector<content_t> out;

    for (const auto& elem : arr) {
        out.push_back(from_json<content_t>(elem));
    }

    return arr;
}

} // namespace deserialization
#endif


} // namespace datalib

} // namespace fhttp