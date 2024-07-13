#pragma once

#include <cstdlib>
#include <sstream>

namespace fhttp {

/// @brief Get an environment variable and convert it to the desired type
template <typename T>
T get_env(const char* key, T default_value) {
    const char* value = std::getenv(key);
    if (value == nullptr) {
        return default_value;
    }

    std::stringstream ss(value);

    T result;
    ss >> result;

    return result;
}

/// @brief Get an environment variable and convert it to the desired type, raises an exception if the conversion fails or the variable is not set
template <typename T>
T get_env_mandatory(const char* key) {
    const char* value = std::getenv(key);
    if (value == nullptr) {
        throw std::runtime_error("Environment variable not set");
    }

    std::stringstream ss(value);

    T result;
    ss >> result;

    return result;
}

} // namespace fhttp