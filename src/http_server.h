#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/regex.hpp>

#include <string>
#include <memory>
#include <unordered_map>
#include <format>
#include <regex>
#include <array>

#include "request.h"
#include "request_parser.h"
#include "response.h"
#include "meta.h"

#include <tuple>
#include <type_traits>

namespace fhttp {

template <typename T>
struct json {
    T data{};
};


std::ostream& operator<<(std::ostream& os, const json<std::string>& json) {
    os << json.data;
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const json<T>&) {
    os << "unknown conversion";
    return os;
}

template <typename handler_t, typename request_body_t = std::string, typename query_params_t = std::string, typename response_body_t = std::string>
struct http_handler {
    using handler_type = handler_t;

    template <typename new_request_body_t>
    using with_request_body = http_handler<handler_t, new_request_body_t, query_params_t, response_body_t>;

    template <typename new_query_t>
    using with_request_query = http_handler<handler_t, request_body_t, new_query_t, response_body_t>;

    template <typename new_response_body_t>
    using with_response_body = http_handler<handler_t, request_body_t, query_params_t, new_response_body_t>;

    using request_t = request<request_body_t, query_params_t>;
    using response_t = response<response_body_t>;

    void handle(const request_t&, response_t&)  {}
};


template <label_literal path, method method_, typename handler_t>
struct route {
    using handler_type = handler_t;
    static constexpr const char* path_value = path.c_str();
    static constexpr method method_value = method_;

    constexpr bool handle_request(const request<std::string>& req, response<std::string>& resp) const {
        if (not matches(req.path, req.method)) {
            return false;
        }

        handler_type handler {};
        typename handler_type::response_t converted_response {};
        const auto converted_request = convert_request<
            typename handler_type::request_t::body_type,
            typename handler_type::request_t::query_params_type
        >(req);

        handler.handle(converted_request, converted_response);

        resp = convert_to_string_response(converted_response);
        return true;
    }

private:
    constexpr bool matches(const std::string& path_to_match, method method) const {
        if (method != method_value) {
            return false;
        }

        static const boost::regex expression { path_value };
        return boost::regex_match(path_to_match, expression);
    }
};


template <typename... Ts>
struct view;

template <typename route_t, typename ... Ts>
struct view<route_t, Ts...> : public view<Ts...> {
    route_t route_instance;

    bool handle_request(const request<std::string>& req, response<std::string>& resp) const {
        if (route_instance.handle_request(req, resp)) {
            return true;
        }

        return view<Ts...>::handle_request(req, resp);
    }
};

template <>
struct view<> {
    bool handle_request(const request<std::string>&, response<std::string>&) const {
        return false;
    }
};

}

struct request_json_params {};
struct query_params {};
struct profile_entity {};

template <typename data_t>
struct generic_response {};

template <typename ... Ts>
struct some {
    // Filter only numeric types and put them in a tuple
    using numeric_types_tuple = decltype(std::tuple_cat(std::declval<std::conditional_t<std::is_arithmetic_v<Ts>, std::tuple<Ts>, std::tuple<>>>()...));

    numeric_types_tuple data;
};

using some_t = some<int, float, std::string>;
void a() {
    some_t t;
    t.data = std::make_tuple(1, 2.0f);
}

struct profile_get_handler: public fhttp::http_handler<profile_get_handler>
    ::with_request_body<fhttp::json<request_json_params>>
    ::with_request_query<query_params>
    ::with_response_body<std::string>
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


namespace fhttp {

struct connection : std::enable_shared_from_this<connection> {
    boost::asio::io_service& io_service;
    boost::asio::io_service::strand strand;
    boost::asio::ip::tcp::socket socket;
    std::array<char, 1024*8> buffer {  };
    request<std::string> current_request { };
    request_parser parser { };
    std::function<void(const request<std::string>&, response<std::string>&)> handle_request;
    bool should_stop { false };

    connection(boost::asio::io_service& io_service, std::function<void(const request<std::string>&, response<std::string>&)>&& handle_request) : 
        io_service { io_service },
        strand { io_service },
        socket { io_service },
        handle_request { handle_request }
    { }

    void start() {
        socket.async_read_some(boost::asio::buffer(buffer),
            strand.wrap(
                boost::bind(&connection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)));
    }

    void handle_read(const boost::system::error_code& e, std::size_t bytes_read) {
        if (e) {
            return;
        }

        boost::tribool result;
        boost::tie(result, boost::tuples::ignore) = parser.parse(
            current_request, buffer.data(), buffer.data() + bytes_read);

        if (result) {
            // handle request
            std::cout << method_to_string(current_request.method) << " " << current_request.path << " " << current_request.version << std::endl;
            response<std::string> response { };
            handle_request(current_request, response);

            if (
                current_request.http_version_major != 1 
                or (
                    current_request.headers.find("Connection") != current_request.headers.end() 
                    and current_request.headers["Connection"] == "close"
                )
            ) {
                should_stop = true;
            }

            const auto response_string = response.to_string();
            boost::asio::async_write(socket, boost::asio::buffer(response_string.data(), response_string.size()),
                strand.wrap(
                    boost::bind(&connection::post_response_sent, shared_from_this(),
                    boost::asio::placeholders::error)));

        } else if (!result) {
            listen_again();
        } else {
            socket.async_read_some(boost::asio::buffer(buffer),
                strand.wrap(
                    boost::bind(&connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)));
        }
    }

    void listen_again() {
        current_request = request<std::string> { };
        parser.reset();
        
        socket.async_read_some(boost::asio::buffer(buffer),
            strand.wrap(
                boost::bind(&connection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)));
    }

    void post_response_sent(const boost::system::error_code& e) {
        if (e or should_stop) {
            return;
        }

        listen_again();
    }
};

template <typename view_t, typename global_state_tuple_t = std::tuple<>, typename thread_local_state_tuple_t = std::tuple<>>
struct server {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::thread_group threadpool;
    std::shared_ptr<connection> connection_instance;
    view_t view_instance { };

    global_state_tuple_t global_state { };

    template <typename ... global_state_ts>
    using with_global_state = server<view_t, std::tuple<global_state_ts ...>, thread_local_state_tuple_t>;

    template <typename ... thread_local_state_ts>
    using with_thread_local_state = server<view_t, global_state_tuple_t, std::tuple<thread_local_state_ts ...>>;

    server(const std::string& host, const std::string& port) : 
        acceptor { io_service },
        connection_instance { std::make_shared<connection>(io_service, [this] (const request<std::string>& req, response<std::string>& resp) {
            if (not view_instance.handle_request(req, resp)) {
                resp.status_code = 404;
                resp.body = "Not found";
                std::cout << "No handler found for path: " << req.path << std::endl;
            }
        }) }
    {
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(host, port);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();

        start_accept();
    }

    void start_accept() {
        acceptor.async_accept(
            connection_instance->socket, 
            boost::bind(&server::handle_accept, this, boost::asio::placeholders::error)
        );
    }

    void start(std::size_t n_threads) {
        for (std::size_t n = 0; n < n_threads; ++n) {
            threadpool.create_thread(
                boost::bind(&boost::asio::io_service::run, &io_service)
            );
        }
    }

    void handle_accept(const boost::system::error_code& e) {
        if (!e) {
            connection_instance->start();
        }
        connection_instance = std::make_shared<connection>(io_service, [this] (const request<std::string>& req, response<std::string>& resp) {
            return view_instance.handle_request(req, resp);
        });
        start_accept();
    }

    void wait() {
        threadpool.join_all();
    }
};

}

struct thread_safe_account_manager {
    std::string get_account_name(const std::string&) {
        return "account name";
    }
};

struct not_thread_safe_friend_manager {
    std::string get_friend_name(const std::string&) {
        return "friend name";
    }
};

void start_http_server(const std::string&, const std::uint16_t& port) {
    std::cout << "Starting server" << std::endl;
    fhttp::server<profile_view>
        ::with_global_state<thread_safe_account_manager>
        ::with_thread_local_state<not_thread_safe_friend_manager> server { "127.0.0.1", std::to_string(port) };
    server.start(1024);
    std::cout << "Waiting for connections" << std::endl;
    server.wait();
    std::cout << "Server stopped" << std::endl;
}