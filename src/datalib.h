#pragma once

#include <string>

namespace fhttp {

namespace datalib {

template <typename T>
constexpr const char* type_name() {
    return T::type_name;
}

template<> constexpr const char* type_name<int>() { return "int"; }
template<> constexpr const char* type_name<float>() { return "float"; }
template<> constexpr const char* type_name<double>() { return "double"; }
template<> constexpr const char* type_name<std::string>() { return "std::string"; }

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

template <typename ... _types>
struct data_pack {
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

    std::tuple<
        _types...
    > fields;

    constexpr std::ostream& json(std::ostream& os) const {
        constexpr std::size_t n = sizeof...(_types);
        os << "{";

        std::apply([&os](auto&& ... args) {
            std::size_t index = 0;
            ([&] {
                constexpr const bool is_numeric = std::is_arithmetic_v<decltype(args.value)>;

                os 
                    << "\"" << args.label << "\": " 
                    << (is_numeric ? "" : "\"") << args.value << (is_numeric ? "" : "\"") << ((index++ < n - 1) ? ", " : "");
            } (), ...);
        }, fields);
        os << "}";
        return os;
    }

    void instrument() {
        std::apply([](auto&& ... args) {
            ((std::cout << args.label << ": " << _types::field_type_name() << std::endl), ...);
        }, fields);
    }
};

}; // namespace datalib

}