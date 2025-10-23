cmake_minimum_required(VERSION 3.23)

find_package(GTest CONFIG REQUIRED)
add_executable(
    parser_tests 
    test_parser.cpp
    "${PROJECT_SOURCE_DIR}/src/parser.cpp"
)
target_include_directories(parser_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(parser_tests PRIVATE
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
  GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(
  parser_tests
  PROPERTIES LABELS "library"
)
