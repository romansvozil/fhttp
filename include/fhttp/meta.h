#pragma once

#include <string>

#define FHTTP_UNUSED(x) (void)(x)

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


// template <typename T>
// inline T from_string(const std::string&) {
//     return T { };
// }

// template <>
// inline std::string from_string<std::string>(const std::string& str) {
//     return str;
// }


// Helper template to transform a type T to a reference type T&
template<typename T>
struct add_reference {
    using type = T&;
};

// Main template that transforms a tuple of types to a tuple of references
template<typename Tuple>
struct tuple_of_references;

// Specialization for std::tuple
template<typename... Types>
struct tuple_of_references<std::tuple<Types...>> {
    using type = std::tuple<typename add_reference<Types>::type...>;
};

// Alias template for ease of use
template<typename Tuple>
using tuple_of_references_t = typename tuple_of_references<Tuple>::type;

// Function to transform a tuple of values to a tuple of references
template<typename... Types>
tuple_of_references_t<std::tuple<Types...>> make_tuple_of_references(std::tuple<Types...>& tpl) {
    return std::apply([](auto&... args) {
        return std::tie(args...);
    }, tpl);
}


// Helper to check if a type is in a tuple
template<typename T, typename Tuple>
struct is_type_in_tuple;

template<typename T>
struct is_type_in_tuple<T, std::tuple<>> : std::false_type {};

template<typename T, typename U, typename... Ts>
struct is_type_in_tuple<T, std::tuple<U, Ts...>> : is_type_in_tuple<T, std::tuple<Ts...>> {};

template<typename T, typename... Ts>
struct is_type_in_tuple<T, std::tuple<T, Ts...>> : std::true_type {};

// Helper to find the index of a type in a tuple
template<typename T, typename Tuple>
struct index_of;

template<typename T, typename... Ts>
struct index_of<T, std::tuple<T, Ts...>> {
    static constexpr std::size_t value = 0;
};

template<typename T, typename U, typename... Ts>
struct index_of<T, std::tuple<U, Ts...>> {
    static constexpr std::size_t value = 1 + index_of<T, std::tuple<Ts...>>::value;
};

// Helper to construct OutputTuple from InputTuple based on matching types
template<typename InputTuple, typename OutputTuple, std::size_t... Is>
OutputTuple filter_and_transform_tuple_impl(InputTuple& t, std::index_sequence<Is...>) {
    return OutputTuple{
        std::get<index_of<typename std::remove_reference<typename std::tuple_element<Is, OutputTuple>::type>::type, InputTuple>::value>(t)...
    };
}

// Main function to filter and remove ignores based on OutputTuple
template<typename InputTuple, typename OutputTuple>
OutputTuple filter_and_remove_ignores(InputTuple& t) {
    return filter_and_transform_tuple_impl<InputTuple, OutputTuple>(
        t, std::make_index_sequence<std::tuple_size<OutputTuple>::value>{});
}


// Helper template to get handlers type definitions
template <auto Method>
struct handler_type_definition;

template <typename ClassType, typename ReturnType, typename... Args, ReturnType (ClassType::*Method)(Args...)>
struct handler_type_definition<Method> {
    using return_type_t = ReturnType;
    using argument_types_t = std::tuple<Args...>;

    // Helper type traits to extract individual argument types
    template <std::size_t N>
    using argument_t = typename std::tuple_element<N, argument_types_t>::type;
    
    using request_t = argument_t<0>;
    using response_t = argument_t<1>;
};

template <typename T, typename = void>
struct has_fields : std::false_type {};

template <typename T>
struct has_fields<T, std::void_t<decltype(std::declval<T>().fields)>> : std::true_type {};


}