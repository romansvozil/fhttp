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
    constexpr static bool handle_request(request<std::string>& req, response<std::string>& resp, global_data_t& global_data, const config_t& config) {
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
    static std::pair<bool, boost::smatch> matches(const std::string& path_to_match, method method) {
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
struct router;

template <typename route_t, typename ... Ts>
struct router<route_t, Ts...> : public router<Ts...> {
    using route_ts = std::tuple<route_t, Ts...>;
    
    template <typename global_data_t, typename config_t>
    bool handle_request(request<std::string>& req, response<std::string>& resp, global_data_t& global_data, const config_t& config) const {
        if (route_t::handle_request(req, resp, global_data, config)) {
            return true;
        }

        return router<Ts...>::handle_request(req, resp, global_data, config);
    }
};

template <>
struct router<> {
    template <typename global_data_t, typename config_t>
    bool handle_request(request<std::string>&, response<std::string>&, global_data_t&, const config_t&) const {
        return false;
    }
};

}

struct query_params {};

namespace fhttp {

struct connection : std::enable_shared_from_this<connection> {
    connection(boost::asio::io_service& io_service, std::function<void(request<std::string>&, response<std::string>&)>&& handle_request, const std::string& server_header);
    void start();
    void set_keep_alive_timeout(std::chrono::steady_clock::duration timeout);
    boost::asio::ip::tcp::socket& get_socket();

private:
    void handle_read(const boost::system::error_code& e, std::size_t bytes_read);
    void listen_again();
    void post_response_sent(const boost::system::error_code& e);
    void close_socket();
    void handle_keep_alive_timeout(const boost::system::error_code& e);
    void start_keep_alive_timer();
    void stop_keep_alive_timer();

private:
    [[maybe_unused]] boost::asio::io_service& io_service;

    /* NOTE: for now unused, since I don't think there are any possibly conflicting operations that would need it */
    boost::asio::io_service::strand strand;

    boost::asio::ip::tcp::socket socket;
    std::array<char, 1024*8> buffer {  };
    request<std::string> current_request { };
    request_parser parser { };
    std::function<void(request<std::string>&, response<std::string>&)> handle_request;
    bool should_stop { false };

    boost::asio::steady_timer keep_alive_timer;
    bool is_keep_alive_timer_running { false };
    std::chrono::steady_clock::duration keep_alive_timeout { };
    const std::string& server_header;

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

template<typename Tuple, typename config_t>
auto create_tuple_from_types(const config_t& config) {
    constexpr std::size_t N = std::tuple_size_v<Tuple>;
    return create_tuple_from_types<Tuple, config_t>(config, std::make_index_sequence<N>{});
}

template <typename router_t, typename config_t = none_config, typename global_state_tuple_t = std::tuple<>, typename thread_local_state_tuple_t = std::tuple<>>
struct server {
    using this_t = server<router_t, config_t, global_state_tuple_t, thread_local_state_tuple_t>;

    server(const std::string& host, const uint16_t port, const config_t& config = config_t { }) : 
        config { config },
        acceptor { io_service },
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
        global_state = { create_tuple_from_types<global_state_tuple_t, config_t>(config) };

    }

    void start_accept() {
        acceptor.async_accept(
            connection_instance->get_socket(), 
            boost::bind(&server::handle_accept, this, boost::asio::placeholders::error)
        );
    }

    void start() {
        initial_connection_instance();
        
        FHTTP_LOG(INFO) << "Starting a server";
        start_accept();
        
        for (std::size_t n = 0; n < n_threads; ++n) {
            threadpool.create_thread(
                boost::bind(&boost::asio::io_service::run, &io_service)
            );
        }
    }

    void initial_connection_instance() {
        connection_instance = std::make_shared<connection>(io_service, [this] (request<std::string>& req, response<std::string>& resp) {
            return router_instance.handle_request(req, resp, unwrap_ref(global_state), this->config);
        }, server_header);
    }

    void handle_accept(const boost::system::error_code& e) {
        if (is_shutting_down) {
            return;
        }

        if (!e) {
            connection_instance->set_keep_alive_timeout(keep_alive_timeout);
            connection_instance->start();
        }
        initial_connection_instance();
        start_accept();
    }

    void join() {
        threadpool.join_all();
    }

    void set_graceful_shutdown_seconds(int seconds) {
        graceful_shutdown_seconds = boost::posix_time::seconds(seconds);
    }

    void set_n_threads(size_t n) {
        n_threads = n;
    }

    void set_keep_alive_timeout(std::chrono::steady_clock::duration timeout) {
        keep_alive_timeout = timeout;
    }

    void set_server_header(const std::string& header) {
        server_header = header;
    }

private:
    const config_t& config { };
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::thread_group threadpool;
    std::shared_ptr<connection> connection_instance;
    router_t router_instance { };
    
    /* Shutdown related */
    boost::posix_time::seconds graceful_shutdown_seconds { 0 };
    bool is_shutting_down { false };
    boost::asio::signal_set signals;
    boost::asio::deadline_timer graceful_shutdown_timer;

    std::chrono::steady_clock::duration keep_alive_timeout { };

    size_t n_threads { 1 };

    std::optional<global_state_tuple_t> global_state { };

    std::string server_header = "FHTTP/0.1";
};

}
