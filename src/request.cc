#include <fhttp/request.h>

namespace fhttp {

const char* method_to_string(method method) {
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

std::optional<method> string_to_method(const std::string& str) {
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

} // namespace fhttp