cmake_minimum_required(VERSION 3.20)

add_executable(parser_example parser/main.cpp)

target_link_libraries(
    parser_example PRIVATE
    itch::parser
    warnings::strict
) 

