add_library(fhttplib request_parser.cc data/json.cc cookies.cc request.cc)
target_compile_features(fhttplib PUBLIC cxx_std_23)
set_target_properties(fhttplib PROPERTIES CXX_STANDARD 23)
