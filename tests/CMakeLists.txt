cmake_minimum_required(VERSION 3.25)

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/_deps")
    include(FetchContent)
    FetchContent_Declare(
        GTest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG "v${GTEST_VERSION}"
    )
    FetchContent_MakeAvailable(GTest)
endif()

add_executable(
  itch_tests
  test_parser.cpp
  test_parser_edge.cpp
  test_messages.cpp
  test_order_book.cpp
  test_order_book_extra.cpp
  test_price_time.cpp
  test_conformance.cpp
  transport/test_transport.cpp
  book/test_book.cpp
  book/test_overlay.cpp
)

target_include_directories(itch_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  itch_tests PRIVATE
  itch::itch
  GTest::gtest
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  itch_tests
  PROPERTIES LABELS "library"
)
