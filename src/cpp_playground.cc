#include <iostream>
#include <utility>
#include <expected> 

#include "http_server.h"

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

namespace example_fields {
    using name = datalib::field<"name", std::string>;
    using age = datalib::field<"age", int>;
    using height = datalib::field<"height", float>;

    struct custom_name_val {
        std::string value;
        
        constexpr custom_name_val() = default;
        constexpr custom_name_val(const std::string& value) : value(value) {}

        constexpr static const char* type_name = "custom_name_val";
    };

    std::ostream& operator<<(std::ostream& os, const custom_name_val& val) {
        os << val.value;
        return os;
    }

    using custom_name = datalib::field<"custom_name", custom_name_val>;

    struct person_entity: public datalib::data_pack<name, age, height, custom_name> {
        using base = datalib::data_pack<name, age, height, custom_name>;
        using base::base;

        constexpr person_entity() = default;
        constexpr person_entity(const std::string& name, int age, float height) : base(name, age, height, custom_name_val { name + " the Submariner" }) {}

        constexpr int months_age() const {
            return get<age>() * 12;
        }
    };
}

int main() {

    datalib::field<"name", std::string> name_field { "Roman Svozil" };
    datalib::field<"cola", std::string> cola_field { "Kofola is much better tho" };

    std::cout << "Label: " << name_field.label << " Value: " << name_field.value << std::endl;
    std::cout << "Label: " << cola_field.label << " Value: " << cola_field.value << std::endl;

    std::cout << "Data pack example: " << std::endl;

    std::string name { "Namor" };

    example_fields::person_entity person {
        name, 21, 1.8f
    };

    std::cout << "Name: "        << person.get<example_fields::name>()              << std::endl;
    std::cout << "Custom Name: " << person.get<example_fields::custom_name>().value << std::endl;
    std::cout << "Age: "         << person.get<example_fields::age>()               << std::endl;
    std::cout << "Height: "      << person.get<example_fields::height>()            << std::endl;
    std::cout << "Age Months: "  << person.months_age()                             << std::endl;

    std::cout << "Instrumenting data pack: " << std::endl;
    person.instrument();

    std::cout << "\n=====JSON=====: " << std::endl;
    person.json(std::cout);

    std::cout << "\n=====HTTP Server=====: " << std::endl;

    std::cout << "Running HTTP server..." << std::endl;
    start_http_server("localhost", 11111);

    return 0;
}