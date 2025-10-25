cmake_minimum_required(VERSION 3.23)

find_program(CLANG_FORMAT_EXE clang-format)
file(GLOB_RECURSE SOURCES_FILES "${CMAKE_SOURCE_DIR}/*.cpp")
file(
    GLOB_RECURSE HEADERS_FILES 
    "${CMAKE_SOURCE_DIR}/*.h" 
    "${CMAKE_SOURCE_DIR}/*.hpp"
)

if (CLANG_FORMAT_EXE)
    add_custom_target(
        format
        COMMENT "Formatting files with clang-format ..."
        COMMAND ${CLANG_FORMAT_EXE} -i ${SOURCES_FILES} ${HEADERS_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    message(STATUS "Add 'format' target")
else()
    message(WARNING "clang-format not found - format target unavailable")
endif()
