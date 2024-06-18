#pragma once

#include <string>
#include <unordered_map>
#include <sstream>
#include <format>

#include <boost/json.hpp>

namespace fhttp {

using json_response = boost::json::object;

template <typename body_t>
struct response {
    using body_type = body_t;

    int status_code{200};
    body_t body{};
    std::string version{"HTTP/1.1"};
    std::unordered_map<std::string, std::string> headers{};

    void send(boost::asio::ip::tcp::socket& socket) {
        std::stringstream ss {};

        std::stringstream body_ss {};
        body_ss << body;
        std::string body = body_ss.str();

        ss << "HTTP/1.1 " << std::to_string(status_code) << " OK\r\n";
        
        headers["Content-Length"] = std::to_string(body.size());

        for (const auto& [key, value] : headers) {
            ss << key << ": " << value << "\r\n";
        }

        ss << "\r\n";
        ss << body; 
    
        std::cout << "sending response: " << body << std::endl;
        boost::asio::write(socket, boost::asio::buffer(ss.str()));
    }

    std::string to_string() {
        std::stringstream ss {};

        std::stringstream body_ss {};
        body_ss << body;
        std::string body = body_ss.str();

        ss << "HTTP/1.1 " << std::to_string(status_code) << " OK\r\n";
        
        headers["Content-Length"] = std::to_string(body.size());

        for (const auto& [key, value] : headers) {
            ss << key << ": " << value << "\r\n";
        }

        ss << "\r\n";
        ss << body; 
        // ss << "\r\n";

        return ss.str();
    }
};

template <typename body_t>
inline response<std::string> convert_to_string_response(const response<body_t>& resp) {
    response<std::string> new_resp {};
    new_resp.status_code = resp.status_code;
    std::stringstream ss_body {};
    ss_body << resp.body;
    new_resp.body = ss_body.str();
    new_resp.version = resp.version;
    new_resp.headers = resp.headers;
    return new_resp;
}

}