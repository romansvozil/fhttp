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
#include "logging.h"
#include "cookies.h"
#include "data/data.h"

#include <tuple>
#include <type_traits>

namespace fhttp {

template <typename T, typename = void>
struct has_description : std::false_type {};

template <typename T>
struct has_description<T, std::void_t<decltype(T::description)>> : std::true_type {};

template <typename T>
const char* get_handler_description() {
    if constexpr (has_description<T>::value) {
        return T::description;
    } else {
        return "This endpoint has no description";
    }
}

template <
    typename config_t,
    typename shared_state_t = std::tuple<>
>
struct http_handler {
    using original_shared_state_t = shared_state_t;
    using references_to_original_shared_state_t = tuple_of_references_t<original_shared_state_t>;

    references_to_original_shared_state_t global_state;
    const config_t& config;

    http_handler() = delete;

    http_handler(const config_t& config, references_to_original_shared_state_t global_state)
        : global_state(global_state)
        , config(config)
    {
    }
};


template <label_literal path, method method_, typename handler_t>
struct route {
    using handler_type = handler_t;
    static constexpr const char* path_value = path.c_str();
    static constexpr method method_value = method_;

    template <typename global_data_t, typename config_t>
    constexpr bool handle_request(request<std::string>& req, response<std::string>& resp, global_data_t& global_data, const config_t& config) const {
        const auto [matched, regex_groups] = matches(req.path, req.method);
        if (not matched) {
            return false;
        }

        req.url_matches = regex_groups;

        auto filtered_global_state_refs = filter_and_remove_ignores<
            global_data_t, 
            typename handler_type::references_to_original_shared_state_t
        >(global_data);

        using handler_definition = handler_type_definition<&handler_type::handle>;

        FHTTP_LOG(INFO) << "Calling a handler with description: " << get_handler_description<handler_type>();
        handler_type handler {config, filtered_global_state_refs};

        std::remove_reference_t<typename handler_definition::response_t> converted_response {};
        const auto converted_request = convert_request<
            typename std::remove_reference_t<typename handler_definition::request_t>::body_type,
            typename std::remove_reference_t<typename handler_definition::request_t>::query_params_type
        >(req);

        handler.handle(converted_request, converted_response);

        resp = convert_to_string_response(converted_response);
        return true;
    }

private:
    std::pair<bool, boost::smatch> matches(const std::string& path_to_match, method method) const {
        boost::smatch what;

        if (method != method_value) {
            return {false, what};
        }

        /// Note: regex expression, can be static, so we won't compile it every time
        /// BUT, not sure if it's thread safe
        static const boost::regex expression { path_value };

        if (!boost::regex_match(path_to_match, what, expression)) {
            return {false, what};
        }

        return {true, what};
    }
};


template <typename... Ts>
struct view;

template <typename route_t, typename ... Ts>
struct view<route_t, Ts...> : public view<Ts...> {
    using route_ts = std::tuple<route_t, Ts...>;
    
    route_t route_instance;

    template <typename global_data_t, typename config_t>
    bool handle_request(request<std::string>& req, response<std::string>& resp, global_data_t& global_data, const config_t& config) const {
        if (route_instance.handle_request(req, resp, global_data, config)) {
            return true;
        }

        return view<Ts...>::handle_request(req, resp, global_data, config);
    }
};

template <>
struct view<> {
    template <typename global_data_t, typename config_t>
    bool handle_request(request<std::string>&, response<std::string>&, global_data_t&, const config_t&) const {
        return false;
    }
};

}

struct request_json_params {};
struct query_params {};
struct profile_entity {};

namespace fhttp {

struct connection : std::enable_shared_from_this<connection> {
    boost::asio::io_service& io_service;
    boost::asio::io_service::strand strand;
    boost::asio::ip::tcp::socket socket;
    std::array<char, 1024*8> buffer {  };
    request<std::string> current_request { };
    request_parser parser { };
    std::function<void(request<std::string>&, response<std::string>&)> handle_request;
    bool should_stop { false };

    connection(boost::asio::io_service& io_service, std::function<void(request<std::string>&, response<std::string>&)>&& handle_request) : 
        io_service { io_service },
        strand { io_service },
        socket { io_service },
        handle_request { handle_request }
    { }

    void start() {
        FHTTP_LOG(INFO) << "Processing incomming connectiong from " << socket.remote_endpoint().address().to_string();
        socket.set_option(boost::asio::ip::tcp::no_delay(true));
        socket.async_read_some(boost::asio::buffer(buffer),
                boost::bind(&connection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
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
            response<std::string> response { };

            if (current_request.headers.count("Cookie")) {
                current_request.cookies.parse(current_request.headers["Cookie"]);
            }

            try {
                handle_request(current_request, response);
            } catch (const std::exception& e) {
                FHTTP_LOG(WARNING) << "Exception caught while handling request: " << e.what();
                response.status_code = 500;
                response.body = "Internal server error";
            }

            if (
                current_request.http_version_major != 1 
                or (
                    current_request.headers.count("Connection") 
                    and current_request.headers["Connection"] == "close"
                )
            ) {
                should_stop = true;
            }

            const auto response_string = response.to_string();
            boost::asio::async_write(socket, boost::asio::buffer(response_string.data(), response_string.size()),
                    boost::bind(&connection::post_response_sent, shared_from_this(),
                    boost::asio::placeholders::error));

        } else if (!result) {
            listen_again();
        } else {
            socket.async_read_some(boost::asio::buffer(buffer),
                    boost::bind(&connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
    }

    void listen_again() {
        current_request = request<std::string> { };
        parser.reset();
        
        socket.async_read_some(boost::asio::buffer(buffer),
                boost::bind(&connection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void post_response_sent(const boost::system::error_code& e) {
        if (e or should_stop) {
            return;
        }

        listen_again();
    }
};

struct none_config { };

template <typename state_t, typename config_t>
std::optional<state_t> create_state(const config_t&) {
    FHTTP_LOG(WARNING) << "Using default create_state function for " << typeid(state_t).name() << " with no config provided";
    return state_t {};
}

template <typename value_t>
value_t unwrap(std::optional<value_t> value) {
    if (value) {
        return *value;
    }

    FHTTP_LOG(FATAL) << "Failed to unwrap value";
    std::unreachable();
}

template <typename value_t>
value_t& unwrap_ref(std::optional<value_t>& value) {
    if (value) {
        return *value;
    }

    FHTTP_LOG(FATAL) << "Failed to unwrap value";
    std::unreachable();
}

// Helper function to create a tuple
template<typename Tuple, typename config_t, std::size_t... Is>
auto create_tuple_from_types(const config_t& config, std::index_sequence<Is...>) {
    return std::make_tuple(unwrap(create_state<std::tuple_element_t<Is, Tuple>, config_t>(config))...);
}

// Main function to create the tuple
template<typename Tuple, typename config_t>
auto create_tuple_from_types(const config_t& config) {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    return create_tuple_from_types<Tuple, config_t>(config, std::make_index_sequence<N>{});
}

template <typename view_t, typename config_t = none_config, typename global_state_tuple_t = std::tuple<>, typename thread_local_state_tuple_t = std::tuple<>>
struct server {
    const config_t& config { };
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::thread_group threadpool;
    std::shared_ptr<connection> connection_instance;
    view_t view_instance { };
    
    /* Shutdown related */
    boost::posix_time::seconds graceful_shutdown_seconds { 0 };
    bool is_shutting_down { false };
    boost::asio::signal_set signals;
    boost::asio::deadline_timer graceful_shutdown_timer;

    size_t n_threads { 1 };

    std::optional<global_state_tuple_t> global_state { };

    template <typename ... global_state_ts>
    using with_global_state = server<view_t, config_t, std::tuple<global_state_ts ...>, thread_local_state_tuple_t>;

    template <typename ... thread_local_state_ts>
    using with_thread_local_state = server<view_t, config_t, global_state_tuple_t, std::tuple<thread_local_state_ts ...>>;

    template <typename new_config_t>
    using with_config = server<view_t, new_config_t, global_state_tuple_t, thread_local_state_tuple_t>;

    using this_t = server<view_t, config_t, global_state_tuple_t, thread_local_state_tuple_t>;

    server(const std::string& host, const uint16_t port, const config_t& config = config_t { }) : 
        config { config },
        acceptor { io_service },
        connection_instance { std::make_shared<connection>(io_service, [this] (request<std::string>& req, response<std::string>& resp) {
            if (not view_instance.handle_request(req, resp, unwrap_ref(global_state), this->config)) {
                resp.status_code = 404;
                resp.body = "Not found";
                std::cout << "No handler found for path: " << req.path << std::endl;
            }
        }) },
        signals(io_service, SIGINT, SIGTERM),
        graceful_shutdown_timer(io_service, graceful_shutdown_seconds)
    {
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(host, std::to_string(port));
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();

        initialize_global_state();
        FHTTP_LOG(INFO) << "Starting a server";
        start_accept();

        signals.async_wait(
            boost::bind(&this_t::graceful_shutdown, this)
        );
    }

    void graceful_shutdown() {
        is_shutting_down = true;

        FHTTP_LOG(INFO) << "Shutting down server gracefully in " << graceful_shutdown_seconds.total_seconds() << " seconds";
        graceful_shutdown_timer = boost::asio::deadline_timer(io_service, graceful_shutdown_seconds);
        graceful_shutdown_timer.async_wait(
            boost::bind(&this_t::shutdown, this)
        );
    }

    void shutdown() {
        FHTTP_LOG(INFO) << "Shutting down server";
        io_service.stop();
    }

    void initialize_global_state() {
        // initialize each part of global_state_tuple_t using the create_state templated function
        global_state = { create_tuple_from_types<global_state_tuple_t, config_t>(config) };

    }

    void start_accept() {
        acceptor.async_accept(
            connection_instance->socket, 
            boost::bind(&server::handle_accept, this, boost::asio::placeholders::error)
        );
    }

    void start() {
        for (std::size_t n = 0; n < n_threads; ++n) {
            threadpool.create_thread(
                boost::bind(&boost::asio::io_service::run, &io_service)
            );
        }
    }

    void handle_accept(const boost::system::error_code& e) {
        if (is_shutting_down) {
            return;
        }

        if (!e) {
            connection_instance->start();
        }
        connection_instance = std::make_shared<connection>(io_service, [this] (request<std::string>& req, response<std::string>& resp) {
            return view_instance.handle_request(req, resp, unwrap_ref(global_state), this->config);
        });
        start_accept();
    }

    void wait() {
        threadpool.join_all();
    }

    void set_graceful_shutdown_seconds(int seconds) {
        graceful_shutdown_seconds = boost::posix_time::seconds(seconds);
    }

    void set_n_threads(size_t n) {
        n_threads = n;
    }
};

}
