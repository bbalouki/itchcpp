cmake_minimum_required(VERSION 3.23)

find_package(benchmark CONFIG REQUIRED)

add_executable(parser_bench parser_bench.cpp)
target_link_libraries(
    parser_bench PRIVATE 
    itch::parser
    benchmark::benchmark
    benchmark::benchmark_main
)
