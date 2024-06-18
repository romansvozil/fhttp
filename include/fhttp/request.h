#pragma once

#include <string>
#include <unordered_map>

#include <boost/asio.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/regex.hpp>

#include "meta.h"
#include "cookies.h"
#include "data/json.h"

namespace fhttp {


template <typename T>
struct json {
    using inner_t = T;
    T data{};

    constexpr const char* content_type() const {
        return "application/json";
    }

    T* operator->() {
        return &data;
    }

    const T* operator->() const {
        return &data;
    }
};

template <typename T>
std::string get_content_type(const json<T>&) {
    return "application/json";
}

template <typename T>
std::string get_content_type(const T&) {
    return "plain/text";
}


inline std::ostream& operator<<(std::ostream& os, const json<std::string>& json) {
    os << json.data;
    return os;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const json<T>& value) {
    if (const auto serialized = datalib::serialization::to_json(value.data); serialized) {
        os << boost::json::serialize(*serialized);
    }
    return os;
}

using json_request = boost::property_tree::ptree;

enum class method {
    get,
    post,
    put,
    patch,
    delete_,
    head,
    options,
    trace,
    connect
};

const char* method_to_string(method method);

std::optional<method> string_to_method(const std::string& str);

template <typename body_t, typename query_params_t = void>
struct request {
    using body_type = body_t;
    using query_params_type = query_params_t;

    method method{};
    std::string path{};
    std::string version{};
    int http_version_major;
    int http_version_minor;
    std::unordered_map<std::string, std::string> headers{};
    body_t body{};
    boost::asio::ip::tcp::endpoint remote_endpoint{};
    cookies cookies{};
    boost::smatch url_matches{};
};

template <typename content_t>
inline content_t from_string(const std::string& str) {
    if constexpr (std::is_same_v<content_t, std::string>) {
        return str;
    } else {
        boost::json::value jv = boost::json::parse(str);
        return { *fhttp::datalib::deserialization::from_json<typename content_t::inner_t>(jv) };
    }
}

template <typename body_t, typename query_params_t = void>
inline request<body_t, query_params_t> convert_request(const request<std::string>& req) {
    request<body_t, query_params_t> new_req {};
    new_req.method = req.method;
    new_req.path = req.path;
    new_req.version = req.version;
    new_req.headers = req.headers;
    new_req.body = from_string<body_t>(req.body);
    new_req.cookies = req.cookies;
    new_req.url_matches = req.url_matches;
    return new_req;
}

// inline boost::property_tree::ptree from_string(const std::string& str) {
//     std::stringstream ss {str};
//     boost::property_tree::ptree pt;

//     try {
//         boost::property_tree::read_json(ss, pt);
//     } catch (const std::exception& e) {
//         // FHTTP_LOG(WARNING) << "Failed to parse json: " << e.what();
//         return {};
//     }

//     return pt;
// }

// template <typename content_t>
// inline content_t from_string(const std::string& str) {
//     boost::json::value jv = boost::json::parse(str);
//     return fhttp::datalib::deserialization::from_json<content_t>(jv);
// }

}