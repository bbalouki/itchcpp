cmake_minimum_required(VERSION 3.23)

find_package(benchmark CONFIG REQUIRED)
add_executable(benchmark bench_main.cpp)
target_link_libraries(
    benchmark PRIVATE 
    itch::parser
    benchmark::benchmark
    benchmark::benchmark_main
)
