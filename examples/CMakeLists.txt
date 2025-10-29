cmake_minimum_required(VERSION 3.20)

add_executable(parser_example parser/main.cpp)
set_cxx_std(parser_example 23)

target_link_libraries(
    parser_example PRIVATE
    itch::parser
    warnings::strict
) 

