#include <fhttp/cookies.h>

namespace fhttp {

void cookies::parse(const std::string& cookie_header) {
    size_t pos = 0;
    while (pos < cookie_header.size()) {
        size_t end_pos = cookie_header.find(';', pos);
        if (end_pos == std::string::npos) {
            end_pos = cookie_header.size();
        }

        size_t eq_pos = cookie_header.find('=', pos);
        if (eq_pos == std::string::npos) {
            pos = end_pos + 1;
            continue;
        }

        std::string key = cookie_header.substr(pos, eq_pos - pos);
        std::string value = cookie_header.substr(eq_pos + 1, end_pos - eq_pos - 1);

        stored_cookies[key] = value;

        pos = end_pos + 1;
    }
}

std::string cookies::get(const std::string& key) {
    return stored_cookies[key];
}

} // namespace fhttp