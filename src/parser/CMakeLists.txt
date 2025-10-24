cmake_minimum_required(VERSION 3.20)

set(LIB_SOURCES 
    parser.cpp
)

add_library(parserlib ${LIB_SOURCES})
setup_target(parserlib) # CXX standard
target_link_libraries(parserlib StrictWarnings)

add_library(itch::parser ALIAS parserlib)
target_include_directories(parserlib PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)
target_include_directories(parserlib PRIVATE 
    ${PROJECT_SOURCE_DIR}/src/parser
)
