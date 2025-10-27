cmake_minimum_required(VERSION 3.23)

option(BUILD_TESTS "Add tests" ON)
option(BUILD_BENCHMARKS "Add benchmark analisys" OFF)
option(BUILD_EXAMPLES "Build some examples" OFF)

set(PROJECT_ENV "DEV" CACHE STRING "Development environment")
set_property(CACHE PROJECT_ENV PROPERTY STRINGS "DEV" "PROD")
message(STATUS "Building for environment: ${PROJECT_ENV}")

function(set_cxx_std target_name standard)
    message(STATUS
        "Setting C++${standard} standard for: ${target_name}"
    )
    target_compile_features(${target_name} PUBLIC cxx_std_${standard})
endfunction()

add_library(warnings INTERFACE)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
     target_compile_options(warnings INTERFACE /W4 /permissive-)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|AppleClang")
    target_compile_options(warnings INTERFACE
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

add_library(warnings::strict ALIAS warnings)
