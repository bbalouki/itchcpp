cmake_minimum_required(VERSION 3.20)

project(ITCH VERSION "1.0.0")

include(${CMAKE_SOURCE_DIR}/cmake/Helpers.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Versions.cmake)

if(${PROJECT_NAME}_ADD_COVERAGE_ANALYSIS)
    add_coverage_analysis()
endif()
if(${PROJECT_NAME}_APPLY_FORMATING)
    apply_formating(${PROJECT_NAME}_USE_PER_FILE_FORMATTING)
endif()
if(${PROJECT_NAME}_APPLY_CLANG_TIDY_GLOBALY)
    apply_clang_tidy_globaly()
endif()
if(${PROJECT_NAME}_BUILD_DOCUMENTATION)
    build_documenation()
endif()
if(${PROJECT_NAME}_ENABLE_ADDRESS_SANITIZER)
    anable_address_sanitizer()
endif()

add_subdirectory(src)

if (${PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if (${PROJECT_NAME}_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
if (${PROJECT_NAME}_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()


