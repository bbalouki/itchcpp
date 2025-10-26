cmake_minimum_required(VERSION 3.23)

option(
  CODE_COVERAGE
  "Enable code coverage instrumentation" OFF
  )

if(
  CODE_COVERAGE 
  AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  )
    message(STATUS "Code coverage enabled")
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING "It's recommended to use Debug build type for code coverage")
    endif()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(
          -O0 -g
          -fcoverage-mapping 
          -fcoverage-mcdc 
          -ftest-coverage 
          --coverage
        )
    else()
        add_compile_options(-O0 -ggdb --coverage)
    endif()        
    add_link_options(--coverage)
endif()

find_program(GCOVR_EXE gcovr)
if(GCOVR_EXE)
  add_custom_target(
    coverage
    COMMAND ${GCOVR_EXE}
      -r "${CMAKE_SOURCE_DIR}"
      --filter "${CMAKE_SOURCE_DIR}/src/"
      --html "coverage.html"
    COMMENT "Generating code coverage report..."
  )
  message(STATUS "Added 'coverage' target")
endif()
