cmake_minimum_required(VERSION 3.23)

find_package(benchmark CONFIG REQUIRED)
add_executable(ITCHBenchmarks bench_main.cpp)
target_link_libraries(
    ITCHBenchmarks PRIVATE 
    itch::parser
    benchmark::benchmark
)
