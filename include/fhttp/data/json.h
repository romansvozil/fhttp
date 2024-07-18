#pragma once

#include <boost/json.hpp>

#include "../meta.h"

#include <optional>

namespace fhttp {

namespace datalib {

namespace utils {

void pretty_print(std::ostream& os, boost::json::value const& jv, std::string* indent = nullptr);

} // namespace utils

namespace serialization {

/* Template to_json definitions */
template <typename content_t>
inline std::enable_if_t<!has_fields<content_t>::value, std::optional<boost::json::value>> to_json(const content_t& content);

template <typename content_t>
inline std::optional<boost::json::value> to_json(const std::vector<content_t>& content);

template <typename content_t>
inline std::optional<boost::json::value> to_json(const std::unordered_map<std::string, content_t>& content);

template <typename content_t>
inline std::enable_if_t<has_fields<content_t>::value, std::optional<boost::json::value>> to_json(const content_t& data_pack_content);

/* Template to_json implementations */

template <typename content_t>
inline std::enable_if_t<!has_fields<content_t>::value, std::optional<boost::json::value>> to_json(const content_t& content) {
    boost::json::value val;
    val = content;
    return val;
}

template <typename content_t>
inline std::optional<boost::json::value> to_json(const std::vector<content_t>& content) {
    boost::json::array arr { };

    for (const auto& value: content) {
        arr.push_back(*to_json(value));
    }

    return arr;
}

template <typename content_t>
inline std::optional<boost::json::value> to_json(const std::unordered_map<std::string, content_t>& content) {
    boost::json::object obj_map;

    for (const auto& [key, value] : content) {
        obj_map[key] = value;
    }

    return obj_map;
}

template <typename content_t>
inline std::enable_if_t<has_fields<content_t>::value, std::optional<boost::json::value>> to_json(const content_t& data_pack_content) {
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

#if 1 // TODO: Finish this
namespace deserialization {

template <typename content_t>
inline std::optional<content_t> from_json(const boost::json::value& json_value) {
    if constexpr (std::is_integral_v<content_t>) {
        return { json_value.as_int64() };
    } else if constexpr (std::is_floating_point_v<content_t>) {
        return { json_value.as_double() };
    } else if constexpr (std::is_same_v<content_t, std::string>) {
        return std::make_optional<std::string>(json_value.as_string());
    } else if constexpr (std::is_same_v<content_t, bool>) {
        return { json_value.as_bool() };
    } else {
        content_t data_pack_content;

        std::apply([&](auto&& ... args) {
            ([&] {
                using field_t = std::decay_t<decltype(args)>;
                using field_value_t = typename field_t::value_type;

                if (const auto deserialized = from_json<field_value_t>(json_value.as_object().at(args.label)); deserialized) {
                    data_pack_content.template set<field_t>({ *deserialized });
                }
            } (), ...);
        }, data_pack_content.fields);

        return std::make_optional(data_pack_content);
    }
}

// template <typename content_t>
// inline std::optional<std::vector<content_t>> from_json(const boost::json::value& json_value) {
//     boost::json::array arr = json_value.as_array();
//     std::vector<content_t> out;

//     for (const auto& elem : arr) {
//         out.push_back(from_json<content_t>(elem));
//     }

//     return arr;
// }

// template <typename content_t>
// inline std::enable_if_t<has_fields<content_t>::value, std::optional<content_t>> from_json(const boost::json::value& json_value) {
//     content_t data_pack_content;

//     std::apply([&](auto&& ... args) {
//         ([&] {
//             using field_t = decltype(args);
//             using field_value_t = typename field_t::value_type;

//             if (const auto deserialized = from_json<field_value_t>(json_value.as_object()[args.label]); deserialized) {
//                 data_pack_content.template set<field_t>(*deserialized);
//             }
//         } (), ...);
//     }, data_pack_content.fields);

//     return std::make_optional(data_pack_content);
// }

} // namespace deserialization
#endif


} // namespace datalib
} // namespace fhttp