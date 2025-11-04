cmake_minimum_required(VERSION 3.23)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    find_package(benchmark CONFIG REQUIRED)
else()
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/_deps")
    include(FetchContent)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG "v${BENCHMARK_VERSION}"
    )
    FetchContent_MakeAvailable(benchmark)
endif()

add_executable(parser_bench parser_bench.cpp)
target_link_libraries(
    parser_bench PRIVATE 
    itch::itch
    benchmark::benchmark
    benchmark::benchmark_main
)
