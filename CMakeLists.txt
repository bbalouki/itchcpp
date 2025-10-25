cmake_minimum_required(VERSION 3.20)
include(${CMAKE_SOURCE_DIR}/cmake/Helpers.cmake)

project(
    ITCHCPP
    VERSION "1.0"
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BUILD_SHARED_LIBS "Build libraries as shared" OFF)


add_executable(itch main.cpp)
target_include_directories(itch PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(itch itch::parser StrictWarnings) 

# include(cmake/Tidy.cmake)
# include(cmake/Format.cmake)
# include(cmake/Sanitize.cmake)
# include(cmake/Coverage.cmake)
# include(cmake/Docs.cmake) 

add_subdirectory(src/parser)

enable_testing()
add_subdirectory(tests)

add_subdirectory(benchmarks)


