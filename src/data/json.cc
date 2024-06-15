#include <fhttp/data/json.h>

namespace fhttp {
namespace datalib {
namespace utils {

/// Taken from https://live.boost.org/doc/libs/1_83_0/libs/json/doc/html/json/examples.html
void pretty_print(std::ostream& os, boost::json::value const& jv, std::string* indent) {
    std::string indent_;
    if(! indent)
        indent = &indent_;
    switch(jv.kind()) {
    case boost::json::kind::object: {
        os << "{\n";
        indent->append(2, ' ');
        auto const& obj = jv.get_object();
        if(! obj.empty()) {
            auto it = obj.begin();
            for(;;)
            {
                os << *indent << boost::json::serialize(it->key()) << " : ";
                pretty_print(os, it->value(), indent);
                if(++it == obj.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 2);
        os << *indent << "}";
        break;
    } case boost::json::kind::array: {
        os << "[\n";
        indent->append(2, ' ');
        auto const& arr = jv.get_array();
        if(! arr.empty()) {
            auto it = arr.begin();
            for(;;) {
                os << *indent;
                pretty_print( os, *it, indent);
                if(++it == arr.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 2);
        os << *indent << "]";
        break;
    } case boost::json::kind::string: {
        os << boost::json::serialize(jv.get_string());
        break;
    }

    case boost::json::kind::uint64:
    case boost::json::kind::int64:
    case boost::json::kind::double_:
        os << jv;
        break;

    case boost::json::kind::bool_:
        if(jv.get_bool())
            os << "true";
        else
            os << "false";
        break;

    case boost::json::kind::null:
        os << "null";
        break;
    }

    if(indent->empty())
        os << "\n";
}

} // namespace utils
} // namespace datalib
} // namespace fhttp
