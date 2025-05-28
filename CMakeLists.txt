cmake_minimum_required(VERSION 3.15)
project(LazyJsonProject VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release"
        FORCE)
endif()

# Sorgenti della libreria
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
include_directories(include)

# Crea la libreria condivisa
add_library(lazyjson SHARED ${LIB_SOURCES})
set_target_properties(lazyjson PROPERTIES OUTPUT_NAME "lazyjson")
set_target_properties(lazyjson PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Opzione per compilare gli esempi
option(BUILD_EXAMPLES "Build the examples" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()