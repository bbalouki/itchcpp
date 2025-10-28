cmake_minimum_required(VERSION 3.20)

add_executable(parser_example parser/main.cpp)
target_include_directories(
    parser_example PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(
    parser_example PRIVATE
    itch::parser
    warnings::strict
) 
set_cxx_std(parser_example 23)
