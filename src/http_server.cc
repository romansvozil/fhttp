#include <fhttp/http_server.h>

namespace fhttp {

connection::connection(boost::asio::io_service& io_service, std::function<void(request<std::string>&, response<std::string>&)>&& handle_request)
    : io_service { io_service }
    , strand { io_service }
    , socket { io_service }
    , handle_request { handle_request }
    , keep_alive_timer { io_service }
{ }

void connection::start() {
    FHTTP_LOG(INFO) << "Processing incomming connectiong from " << socket.remote_endpoint().address().to_string();
    socket.set_option(boost::asio::ip::tcp::no_delay(true));
    socket.async_read_some(boost::asio::buffer(buffer),
            boost::bind(&connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void connection::set_keep_alive_timeout(std::chrono::steady_clock::duration timeout) {
    keep_alive_timeout = timeout;
}

void connection::handle_read(const boost::system::error_code& e, std::size_t bytes_read) {
    if (e) {
        return;
    }

    if (is_keep_alive_timer_running) {
        stop_keep_alive_timer();
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

void connection::listen_again() {
    current_request = request<std::string> { };
    parser.reset();
    
    socket.async_read_some(boost::asio::buffer(buffer),
        boost::bind(&connection::handle_read, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void connection::post_response_sent(const boost::system::error_code& e) {
    if (e or should_stop) {
        close_socket();
        return;
    }

    start_keep_alive_timer();
    listen_again();
}

void connection::close_socket() {
    boost::system::error_code ignored_ec;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket.close(ignored_ec);
}

void connection::handle_keep_alive_timeout(const boost::system::error_code& e) {
    if (e) {
        return;
    }

    boost::system::error_code remote_endpoint_ec;

    const auto remote_endpoint = socket.remote_endpoint(remote_endpoint_ec);
    std::string remote_endpoint_str;
    if (remote_endpoint_ec) {
        remote_endpoint_str = "unknown";
    } else {
        remote_endpoint_str = remote_endpoint.address().to_string();
    }
    FHTTP_LOG(INFO) << "Connection is going to be closed due to keep-alive timeout [ip: " << remote_endpoint_str << "]";
    close_socket();
}

void connection::start_keep_alive_timer() {
    is_keep_alive_timer_running = true;
    keep_alive_timer.expires_after(keep_alive_timeout);
    keep_alive_timer.async_wait(
        boost::bind(&connection::handle_keep_alive_timeout, shared_from_this(), boost::asio::placeholders::error)
    );
}

void connection::stop_keep_alive_timer() {
    is_keep_alive_timer_running = false;
    keep_alive_timer.cancel();
}

boost::asio::ip::tcp::socket& connection::get_socket() {
    return socket;
}


} // namespace fhttp