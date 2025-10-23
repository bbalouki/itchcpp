cmake_minimum_required(VERSION 3.20)

project(
    ITCHCPP
    VERSION "1.0"
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BUILD_SHARED_LIBS "Build libraries as shared" OFF)

add_executable(itch src/parser.cpp main.cpp )
target_include_directories(itch PUBLIC ${PROJECT_SOURCE_DIR}/include)


# Create an INTERFACE target to hold our warnings flags
# add_library(StrictWarnings INTERFACE)

# # Attach compiler options to this target
# if (MSVC)
#     target_compile_options(StrictWarnings INTERFACE /W4)
# else()
#     target_compile_options(StrictWarnings INTERFACE
#         -Wall 
#         -Wextra 
#         -Wpedantic
#     )
# endif()

# include(cmake/Tidy.cmake)
# include(cmake/Format.cmake)
# include(cmake/Sanitize.cmake)
# include(cmake/Coverage.cmake)
# include(cmake/Docs.cmake)

# add_subdirectory(src/**)

enable_testing()
add_subdirectory(tests)

# add_subdirectory(benchmarks)


