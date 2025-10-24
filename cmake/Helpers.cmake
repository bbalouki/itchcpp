cmake_minimum_required(VERSION 3.23)

function(setup_target target_name)
    message(STATUS
        "Setting C++23 standard for :${target_name}"
    )
    target_compile_features(${target_name} PUBLIC cxx_std_23)
endfunction()

add_library(StrictWarnings INTERFACE)
if (MSVC)
    target_compile_options(StrictWarnings INTERFACE /W4)
else()
    target_compile_options(StrictWarnings INTERFACE
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()
