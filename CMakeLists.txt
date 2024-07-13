cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify the minimum compiler version that supports C++23
set(CMAKE_CXX_COMPILER_VERSION 17.0) # Update this based on your actual compiler version

add_compile_options(-Wall -Wextra -Wpedantic -Werror -O3)

project(
  cpp-playground
  VERSION 1.0
  LANGUAGES CXX)

include_directories(include)

find_package(Boost 1.81.0 COMPONENTS filesystem regex thread chrono date_time json) 

add_subdirectory(src)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    message ("-- Boost libraries found: ${Boost_LIBRARIES}")
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(fhttplib ${Boost_LIBRARIES})
endif()

add_subdirectory(examples/basic_http_server)

# add_executable(cpp-playground src/cpp_playground.cc src/request_parser.cc src/data/json.cc)
# target_compile_features(cpp-playground PUBLIC cxx_std_23)
# set_target_properties(cpp-playground PROPERTIES CXX_STANDARD 23)

