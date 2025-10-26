cmake_minimum_required(VERSION 3.23)

function(set_cxx_std target_name standard)
    message(STATUS
        "Setting C++${standard} standard for: ${target_name}"
    )
    target_compile_features(${target_name} PUBLIC cxx_std_${standard})
endfunction()

add_library(warnings INTERFACE)
if (MSVC)
    target_compile_options(warnings INTERFACE /W4)
else()
    target_compile_options(warnings INTERFACE
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

add_library(warnings::strict ALIAS warnings)
