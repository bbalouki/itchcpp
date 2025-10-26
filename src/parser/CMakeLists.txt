cmake_minimum_required(VERSION 3.20)

set(LIB_SOURCES 
    parser.cpp
)

add_library(parserlib ${LIB_SOURCES})
set_cxx_std(parserlib 23)
target_link_libraries(parserlib warnings::strict)

add_library(itch::parser ALIAS parserlib)
target_include_directories(parserlib PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)
target_include_directories(parserlib PRIVATE 
    ${PROJECT_SOURCE_DIR}/src/parser
)
