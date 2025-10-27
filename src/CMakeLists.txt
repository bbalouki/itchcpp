cmake_minimum_required(VERSION 3.20)


add_library(messageslib messages.cpp)
set_cxx_std(messageslib 23)
target_link_libraries(messageslib warnings::strict)
add_library(parserlib parser.cpp)
set_cxx_std(parserlib 23)
target_link_libraries(parserlib warnings::strict messageslib)

add_library(itch::parser ALIAS parserlib)
add_library(itch::messages ALIAS messageslib)
target_include_directories(parserlib PUBLIC 
    ${PROJECT_SOURCE_DIR}/include/itch
)
target_include_directories(messageslib PUBLIC
    ${PROJECT_SOURCE_DIR}/include/itch
)

