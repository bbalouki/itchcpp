cmake_minimum_required(VERSION 3.20)

add_executable(parser parser/main.cpp)
target_include_directories(
    parser PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(
    parser PRIVATE
    itch::parser 
    warnings::strict
) 
set_cxx_std(parser 23)
