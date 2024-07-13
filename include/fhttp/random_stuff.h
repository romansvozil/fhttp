#pragma once

/* TODO: TO BE REMOVED */

#include "datalib.h"
#include "logging.h"


namespace example_fields {

using name = fhttp::datalib::field<"name", std::string>;
using age = fhttp::datalib::field<"age", int>;
using height = fhttp::datalib::field<"height", float>;

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

using custom_name = fhttp::datalib::field<"custom_name", custom_name_val>;

struct person_entity: public fhttp::datalib::data_pack<name, age, height, custom_name> {
    using base = fhttp::datalib::data_pack<name, age, height, custom_name>;
    using base::base;

    constexpr person_entity() = default;
    constexpr person_entity(const std::string& name, int age, float height) : base(name, age, height, custom_name_val { name + " the Submariner" }) {}

    constexpr int months_age() const {
        return get<age>() * 12;
    }
};

}

int main() {
    fhttp::datalib::field<"name", std::string> name_field { "Roman Svozil" };
    fhttp::datalib::field<"cola", std::string> cola_field { "Kofola is much better tho" };

    FHTTP_LOG(INFO) << "Label: " << name_field.label << " Value: " << name_field.value;
    FHTTP_LOG(INFO) << "Label: " << cola_field.label << " Value: " << cola_field.value;

    FHTTP_LOG(INFO) << "Data pack example: ";

    std::string name { "Namor" };

    example_fields::person_entity person {
        name, 21, 1.8f
    };

    FHTTP_LOG(INFO) << "Name: "        << person.get<example_fields::name>();
    FHTTP_LOG(INFO) << "Custom Name: " << person.get<example_fields::custom_name>().value;
    FHTTP_LOG(INFO) << "Age: "         << person.get<example_fields::age>();
    FHTTP_LOG(INFO) << "Height: "      << person.get<example_fields::height>();
    FHTTP_LOG(INFO) << "Age Months: "  << person.months_age();

    FHTTP_LOG(INFO) << "Instrumenting data pack: ";
    person.instrument();

    FHTTP_LOG(INFO) << "=====JSON=====: ";
    person.json(FHTTP_LOG(INFO));

    FHTTP_LOG(INFO) << "=====HTTP Server=====: ";

    FHTTP_LOG(INFO) << "Running HTTP server...";
    
}