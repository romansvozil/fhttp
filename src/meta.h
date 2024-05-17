#pragma once

#include <string>

namespace fhttp {

template<size_t N>
struct label_literal {
    constexpr label_literal(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }

    constexpr const char* c_str() const {
        return value;
    }
    
    char value[N] = { 0 };
};


template <typename T>
inline T from_string(const std::string&) {
    return T { };
}

template <>
inline std::string from_string<std::string>(const std::string& str) {
    return str;
}

}