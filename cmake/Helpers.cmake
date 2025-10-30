cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(${PROJECT_NAME}_CXX_STANDARD 20 CACHE STRING "C++ standard for the project")
set(CMAKE_CXX_STANDARD ${${PROJECT_NAME}_CXX_STANDARD})

option(${PROJECT_NAME}_BUILD_TESTS "Add tests" OFF)
option(${PROJECT_NAME}_BUILD_BENCHMARKS "Add benchmark analysis" OFF)
option(${PROJECT_NAME}_BUILD_EXAMPLES "Build some examples" OFF)

set(${PROJECT_NAME}_PROJECT_ENV "DEV" CACHE STRING "Development environment")
set_property(CACHE ${PROJECT_NAME}_PROJECT_ENV PROPERTY STRINGS "DEV" "PROD")
message(STATUS "Building for environment: ${${PROJECT_NAME}_PROJECT_ENV}")

option(${PROJECT_NAME}_ADD_COVERAGE_ANALYSIS "Enable coverage analisys" OFF)
option(${PROJECT_NAME}_APPLY_FORMATING "Apply formating with clang-format" OFF)
option(${PROJECT_NAME}_USE_PER_FILE_FORMATTING "For very large projects" OFF)
option(${PROJECT_NAME}_APPLY_CLANG_TIDY_GLOBALY "Apply clang tidy globaly" OFF)
option(${PROJECT_NAME}_BUILD_DOCUMENTATION "Build documenation with Doxygen" OFF)
option(${PROJECT_NAME}_ENABLE_ADDRESS_SANITIZER "Enable Address Sanitizer" OFF)

option(USE_PER_FILE_FORMATTING "Apply formating on one file at the time" OFF)

######################################################
function(set_cxx_std target_name standard)
    message(STATUS
        "Setting C++${standard} standard for: ${target_name}"
    )
    target_compile_features(${target_name} PUBLIC cxx_std_${standard})
endfunction()

######################################################
function(configure_visibility target_name)
    if (WIN32 AND BUILD_SHARED_LIBS)
        set_target_properties(${target_name} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    elseif (BUILD_SHARED_LIBS)
        set_target_properties(${target_name} PROPERTIES
            CXX_VISIBILITY_PRESET default
        )
    endif()
endfunction()

######################################################
add_library(warnings INTERFACE)
add_library(warnings::strict ALIAS warnings)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
     target_compile_options(warnings INTERFACE 
        /W4                     # Warning level 4
        /WX                     # Treat warnings as errors
        /EHsc                   # Enable C++ exception handling (standard)
        /permissive-            # Enforce standard C++ conformance
        /options:strict         # unrecognized compiler options are an error
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|AppleClang")
    target_compile_options(warnings INTERFACE
        -Wall                       # Enable most warning messages.
        -Wpedantic                  # Enforces strict adherence to the C++ standard.
        -Werror                     # Treat warnings as errors
        -Wconversion                # Warns about implicit type conversions that may cause loss of data.
        -Wpedantic                  # Issue warnings needed for strict compliance to the standard.
        -pedantic-errors            # Like -pedantic but issue them as errors.
        -Wsign-conversion           # Warns about implicit conversions between signed and unsigned types.
        -Wshadow                    # Warn when one variable shadows another (globally).
        -Wshadow=local              # Warn when one local variable shadows another local variable or parameter.
    )
endif()

###################################################
function(apply_clang_tidy TARGET_NAME)
    find_program(CLANG_TIDY_EXE "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy not found! C++ linting will be disabled.")
        return()
    endif()
    cmake_parse_arguments(TIDY "" "EXTRA_ARGS" "" ${ARGN})
    set(LINTING_COMMAND ${CLANG_TIDY_EXE})
    if(TIDY_EXTRA_ARGS)
        list(APPEND LINTING_COMMAND ${TIDY_EXTRA_ARGS})
    endif()
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_CLANG_TIDY "${LINTING_COMMAND}"
    )
    message(STATUS "Enabled clang-tidy for target: ${TARGET_NAME}")
endfunction()

###################################################
function(apply_clang_tidy_globaly)
  find_program(CLANG_TIDY_EXE "clang-tidy")

  if(CLANG_TIDY_EXE)
      message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
      set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
  else()
      message(WARNING "clang-tidy not found! C++ linting will be disabled.")
  endif()
endfunction()

####################################################
function(add_coverage_analysis)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
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
endfunction()

####################################################
function(build_documenation)
    find_package(Doxygen)
    if (Doxygen_FOUND)
        message(
            STATUS 
            "Found Doxygen Version " "${DOXYGEN_VERSION} " 
            "at ${DOXYGEN_EXECUTABLE}"
        )
        configure_file(
            "${PROJECT_SOURCE_DIR}/docs/Doxyfile.in"
            "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
            @ONLY
        )
        doxygen_add_docs(
            docs 
            CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
            COMMENT "Generating docs with Doxygen ..."
        )
        message(STATUS "Add 'docs' target")
    endif()
endfunction()

#####################################################
function(apply_formating USE_PER_FILE_LOGIC)
    find_program(CLANG_FORMAT_EXE clang-format)

    if (NOT CLANG_FORMAT_EXE)
        message(WARNING "clang-format not found - 'format' target will not be created.")
        return()
    endif()

    file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/*.cpp")
    file(GLOB_RECURSE HEADERS "${CMAKE_SOURCE_DIR}/*.h" "${CMAKE_SOURCE_DIR}/*.hpp")
    list(APPEND ALL_FILES ${SOURCES} ${HEADERS})

    if(NOT ALL_FILES)
        message(WARNING "No source (.cpp) or header (.h, .hpp) files found. 'format' target will be empty.")
        add_custom_target(format COMMENT "No source files found to format.")
        return()
    endif()

    if (USE_PER_FILE_LOGIC)
        message(STATUS "Add 'format' target (per-file mode for large projects)")
        add_custom_target(
            format
            COMMENT "Formatting files one-by-one with clang-format ..."
        )
        foreach(file ${ALL_FILES})
            add_custom_command(
                TARGET format
                COMMAND ${CLANG_FORMAT_EXE} -i "${file}"
                COMMENT "Formatting ${file}"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            )
        endforeach()
    else()
        message(STATUS "Add 'format' target (single command mode)")
        add_custom_target(
            format
            COMMENT "Formatting all files at once with clang-format ..."
            COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
    message(STATUS "To format your code, run: cmake --build . --target format")

endfunction()

#####################################################
function(anable_address_sanitizer)
  set(SANITIZER_SUPPORTED OFF)
  if (MINGW)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(SANITIZER_SUPPORTED OFF)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      set(SANITIZER_SUPPORTED ON)
    endif()
  elseif (MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(SANITIZER_SUPPORTED ON)
  endif()

  if (SANITIZER_SUPPORTED)
    message(STATUS "Enabling AddressSanitizer")
    if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
      add_compile_options(/fsanitize=address /Zi)
      add_link_options(/INCREMENTAL:NO)
    else()
      add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
      add_link_options(-fsanitize=address)
    endif()
  else()
    message(WARNING "Sanitizers are NOT supported with ${CMAKE_CXX_COMPILER_ID} on MinGW. Skipping.")
  endif()  
endfunction()
