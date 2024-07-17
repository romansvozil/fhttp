#include <fhttp/logging.h>

#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace fhttp {
namespace logging {

std::string get_current_time() {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << '.' 
              << std::setw(3) << std::setfill('0') << now_ms.count();
    return ss.str();
}

log_message::log_message(severity_level severity, const char* file_name, const char* func_name, int line) :
	severity(severity), file_name(file_name), func_name(func_name), line(line) {
}

log_message::~log_message() {
	print_log_message();
}

void log_message::print_log_message() {
	fprintf(stderr, "%s %s: %s '%s' {%s:%d}\n", get_current_time().c_str(), severity_names[severity], func_name, str().c_str(), file_name, line);
}

log_message_fatal::log_message_fatal(const char* file_name, const char* func_name, int line) :
	log_message(FATAL, file_name, func_name, line) {
}

log_message_fatal::~log_message_fatal() {
	print_log_message();
	abort();
}

} // namespace logging
} // namespace fhttp
