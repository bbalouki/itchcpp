cmake_minimum_required(VERSION 3.20)

include(${CMAKE_SOURCE_DIR}/cmake/Helpers.cmake)

project(ITCH VERSION "1.0.0" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BUILD_SHARED_LIBS "Build libraries as shared" OFF)

if(PROJECT_ENV STREQUAL "DEV")
    include(cmake/Tidy.cmake)
    include(cmake/Format.cmake)
    include(cmake/Coverage.cmake)
endif()

include(cmake/Sanitize.cmake)
add_subdirectory(src)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

