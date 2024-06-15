#pragma once

#include <unordered_map>
#include <string>

namespace fhttp {

struct cookies {
    cookies() = default;

    /// @brief Parses cookies from a cookie header
    /// @param cookie_header 
    void parse(const std::string& cookie_header);

    /// @brief Retrieves a cookie by its key
    /// @param key 
    /// @return 
    std::string get(const std::string& key);

private:
    std::unordered_map<std::string, std::string> stored_cookies;
};

} // namespace fhttp
