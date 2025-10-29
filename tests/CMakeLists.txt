cmake_minimum_required(VERSION 3.23)

find_package(GTest CONFIG REQUIRED)

add_executable(
  itch_tests
  test_parser.cpp
  test_messages.cpp
)

target_link_libraries(
  itch_tests PRIVATE
  itch::parser
  GTest::gtest
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  itch_tests
  PROPERTIES LABELS "library"
)
