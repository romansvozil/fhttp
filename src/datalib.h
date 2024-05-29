#pragma once

#include <string>

#include <boost/json.hpp>

namespace json = boost::json;

namespace fhttp {

namespace datalib {

template <typename T>
constexpr const char* type_name() {
    return T::type_name;
}

template<> constexpr const char* type_name<int>() { return "int"; }
template<> constexpr const char* type_name<float>() { return "float"; }
template<> constexpr const char* type_name<double>() { return "double"; }
template<> constexpr const char* type_name<std::string>() { return "string"; }

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

template <internal::label_literal label_value, typename _value_type>
struct field {
public:
    using value_type = _value_type;

    constexpr field() = default;
    constexpr field(const value_type& value) : value(value) {}

    static constexpr const char* field_type_name() {
        return type_name<value_type>();
    }

public:
    static constexpr const char* label = label_value.c_str();
    value_type value;
};

#define FHTTP_FIELD(field_type_name, label, value_type) struct field_type_name : fhttp::datalib::field<label, value_type> {  }

template <typename ... _types>
struct data_pack {
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

    void instrument() {
        std::apply([](auto&& ... args) {
            ((std::cout << args.label << ": " << _types::field_type_name() << std::endl), ...);
        }, fields);
    }
};

template <typename content_t>
std::enable_if_t<!has_fields<content_t>::value, boost::json::value> to_json(const content_t& content) {
    boost::json::value val;
    val = content;
    return val;
}

template <typename content_t>
boost::json::value to_json(const std::vector<content_t>& content) {
    boost::json::array arr {content.begin(), content.end()};

    return arr;
}

template <typename content_t>
boost::json::value to_json(const std::unordered_map<std::string, content_t>& content) {
    boost::json::object obj_map;

    for (const auto& [key, value] : content) {
        obj_map[key] = value;
    }

    return obj_map;
}

template <typename content_t>
std::enable_if_t<has_fields<content_t>::value, boost::json::value> to_json(const content_t& data_pack_content) {
    boost::json::object obj;

    std::apply([&obj](auto&& ... args) {
        ([&] {
            obj[args.label] = to_json(args.value);
        } (), ...);
    }, data_pack_content.fields);

    return obj;
}

}; // namespace datalib

}