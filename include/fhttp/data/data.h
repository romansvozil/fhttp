#pragma once

#include <string>

#include <boost/json.hpp>

namespace json = boost::json;

namespace fhttp {

namespace datalib {

template <typename T>
constexpr const char* type_name() requires requires { T::type_name; } {
    return T::type_name;
}

template <typename T>
constexpr const char* type_name() {
    return "unknown";
}

template<> constexpr const char* type_name<int>() { return "integer"; }
template<> constexpr const char* type_name<float>() { return "number"; }
template<> constexpr const char* type_name<double>() { return "number"; }
template<> constexpr const char* type_name<std::string>() { return "string"; }
template<> constexpr const char* type_name<bool>() { return "boolean"; }

namespace internal {

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

}; // namespace internal

struct data_pack_base {};

template <internal::label_literal label_value, typename _value_type, internal::label_literal description_value = "">
struct field {
public:
    using value_type = _value_type;
    static constexpr bool has_value_type_data_pack = std::is_base_of_v<data_pack_base, value_type>;

    constexpr field() = default;
    constexpr field(const value_type& value) : value(value) {}

    static constexpr const char* field_type_name() {
        return type_name<value_type>();
    }

public:
    static constexpr const char* label = label_value.c_str();
    static constexpr const char* description = description_value.c_str();
    value_type value;
};

#define FHTTP_FIELD(field_type_name, label, value_type) struct field_type_name : fhttp::datalib::field<label, value_type> {  }

template <typename ... _types>
struct data_pack: data_pack_base {
    using tuple_type_t = std::tuple<_types...>;

    constexpr data_pack() = default;
    constexpr data_pack(_types&& ... args) : fields(std::forward<_types>(args)...) {}

    template <typename T>
    constexpr T::value_type& get() {
        return std::get<T>(fields).value;
    }

    template <typename T>
    constexpr const T::value_type& get() const {
        return std::get<T>(fields).value;
    }

    template <typename T>
    constexpr void set(const T::value_type& value) {
        std::get<T>(fields).value = value;
    }

    tuple_type_t fields;
};

}; // namespace datalib

}