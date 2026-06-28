cmake_minimum_required(VERSION 3.25)

# libFuzzer ships with Clang. On other compilers the fuzz target is simply not
# created, so enabling the option elsewhere is a harmless no-op.
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Fuzzers require Clang (libFuzzer); skipping fuzz targets.")
    return()
endif()

# Instrument the library itself so the fuzzer gets coverage feedback through it,
# while leaving the main (non-fuzz) builds of the library untouched.
target_compile_options(itch PRIVATE -fsanitize=fuzzer-no-link,address,undefined)

add_executable(parser_fuzzer parser_fuzzer.cpp)
target_link_libraries(parser_fuzzer PRIVATE itch::itch)
target_compile_options(parser_fuzzer PRIVATE -g -fsanitize=fuzzer,address,undefined)
target_link_options(parser_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)

message(STATUS "Added 'parser_fuzzer' target")
