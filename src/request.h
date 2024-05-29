#pragma once

#include <string>
#include <unordered_map>

#include <boost/asio.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "meta.h"
#include "cookies.h"
// #include "logging.h"

namespace fhttp {

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

inline constexpr const char* method_to_string(method method) {
    switch (method) {
        case method::get: return "GET";
        case method::post: return "POST";
        case method::put: return "PUT";
        case method::patch: return "PATCH";
        case method::delete_: return "DELETE";
        case method::head: return "HEAD";
        case method::options: return "OPTIONS";
        case method::trace: return "TRACE";
        case method::connect: return "CONNECT";
    }
}

inline constexpr std::optional<method> string_to_method(const std::string& str) {
    if (str == "GET") return method::get;
    if (str == "POST") return method::post;
    if (str == "PUT") return method::put;
    if (str == "PATCH") return method::patch;
    if (str == "DELETE") return method::delete_;
    if (str == "HEAD") return method::head;
    if (str == "OPTIONS") return method::options;
    if (str == "TRACE") return method::trace;
    if (str == "CONNECT") return method::connect;
    return std::nullopt;
}

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
};

template <typename body_t, typename query_params_t = void>
inline request<body_t, query_params_t> convert_request(const request<std::string>& req) {
    request<body_t, query_params_t> new_req {};
    new_req.method = req.method;
    new_req.path = req.path;
    new_req.version = req.version;
    new_req.headers = req.headers;
    new_req.body = from_string<body_t>(req.body);
    new_req.cookies = req.cookies;
    return new_req;
}

template <>
inline boost::property_tree::ptree from_string(const std::string& str) {
    std::stringstream ss {str};
    boost::property_tree::ptree pt;

    try {
        boost::property_tree::read_json(ss, pt);
    } catch (const std::exception& e) {
        // FHTTP_LOG(WARNING) << "Failed to parse json: " << e.what();
        return {};
    }

    return pt;
}

}