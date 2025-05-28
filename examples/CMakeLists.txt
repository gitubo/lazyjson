cmake_minimum_required(VERSION 3.15)
project(LazyJsonExamples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")

# Trova nlohmann_json installata a sistema
find_package(nlohmann_json REQUIRED)

find_path(RAPIDJSON_INCLUDE_DIR rapidjson/document.h)
include_directories(${RAPIDJSON_INCLUDE_DIR})

file(GLOB_RECURSE EXAMPLE_SOURCES "*.cpp")

foreach(example_source ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_source} NAME_WE)
    add_executable(${example_name} ${example_source})

    # Link alle librerie
    target_link_libraries(${example_name}
        PRIVATE
        lazyjson
        nlohmann_json::nlohmann_json
    )

    link_directories(${CMAKE_BINARY_DIR}/lib)
endforeach()
