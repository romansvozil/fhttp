#pragma once

#include <unordered_map>
#include <string>

namespace fhttp {

struct cookies {
    cookies() = default;

    void parse(const std::string& cookie_header) {
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

    std::string get(const std::string& key) {
        return stored_cookies[key];
    }

private:
    std::unordered_map<std::string, std::string> stored_cookies;
};

} // namespace fhttp
