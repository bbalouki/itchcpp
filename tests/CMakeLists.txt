cmake_minimum_required(VERSION 3.23)

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
  test_messages.cpp
  test_order_book.cpp
)

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
