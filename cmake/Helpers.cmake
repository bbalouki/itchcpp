cmake_minimum_required(VERSION 3.23)

option(ITCH_BUILD_TESTS "Add tests" OFF)
option(ITCH_BUILD_BENCHMARKS "Add benchmark analisys" OFF)
option(ITCH_BUILD_EXAMPLES "Build some examples" OFF)

set(ITCH_PROJECT_ENV "DEV" CACHE STRING "Development environment")
set_property(CACHE ITCH_PROJECT_ENV PROPERTY STRINGS "DEV" "PROD")
message(STATUS "Building for environment: ${ITCH_PROJECT_ENV}")

function(set_cxx_std target_name standard)
    message(STATUS
        "Setting C++${standard} standard for: ${target_name}"
    )
    target_compile_features(${target_name} PUBLIC cxx_std_${standard})
endfunction()

function(configure_visibility target_name)
    if (WIN32 AND BUILD_SHARED_LIBS)
        set_target_properties(${target_name} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    elseif (BUILD_SHARED_LIBS)
        set_target_properties(${target_name} PROPERTIES
            CXX_VISIBILITY_PRESET default
        )
    endif()
endfunction()


add_library(warnings INTERFACE)
add_library(warnings::strict ALIAS warnings)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
     target_compile_options(warnings INTERFACE 
        /W4                     # Warning level 4
        /WX                     # Treat warnings as errors
        /permissive-            # Enforce standard C++ conformance
        /options:strict         # unrecognized compiler options are an error
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|AppleClang")
    target_compile_options(warnings INTERFACE
        -Wall                   # Enable most warning messages.
        -Wpedantic              # Enforces strict adherence to the C++ standard.
        -Werror                 # Treat warnings as errors
        -Wconversion            # Warns about implicit type conversions that may cause loss of data.
        -pedantic-errors        # Like -pedantic but issue them as errors.
        -Wsign-conversion       # Warns about implicit conversions between signed and unsigned types.
        -Wshadow                # Warn when one variable shadows another (globally).
        -Wshadow=local          # Warn when one local variable shadows another local variable or parameter.
    )
endif()
