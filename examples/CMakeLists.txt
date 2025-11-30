cmake_minimum_required(VERSION 3.20)

add_executable(parser_example parser/parser_example.cpp)

target_link_libraries(
    parser_example PRIVATE
    itch::itch
    warnings::strict
)

add_executable(order_book_example order_book/order_book_example.cpp)

target_link_libraries(
    order_book_example PRIVATE
    itch::itch
    warnings::strict
)
