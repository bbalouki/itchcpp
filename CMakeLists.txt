cmake_minimum_required(VERSION 3.20)

project(ITCH VERSION "1.0.0" LANGUAGES CXX)

set(ITCH_CXX_STANDARD 20 CACHE STRING "C++ standard for the project")
set(CMAKE_CXX_STANDARD ${ITCH_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/Helpers.cmake)

if(ITCH_PROJECT_ENV STREQUAL "DEV")
    include(cmake/Tidy.cmake)
    include(cmake/Format.cmake)
    include(cmake/Coverage.cmake)
endif()

include(cmake/Sanitize.cmake)
add_subdirectory(src)

if (ITCH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (ITCH_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if (ITCH_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
