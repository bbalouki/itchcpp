cmake_minimum_required(VERSION 3.25)

include(GNUInstallDirs)

# Step 1: Add targets and properties
add_library(
    itch
    parser.cpp
    messages.cpp
    order_book.cpp
    transport/moldudp64.cpp
    transport/soupbintcp.cpp
    transport/pcap.cpp
    book/l3_book.cpp
    book/book_manager.cpp
    io/csv_sink.cpp
    io/arrow_export.cpp
    encoder.cpp
    replay.cpp
)

# Apache Arrow / Parquet columnar export is optional and off by default so the
# core library stays dependency-free. The arrow_export.cpp translation unit is
# empty unless ITCH_WITH_ARROW is defined.
if(${PROJECT_NAME}_WITH_ARROW)
    find_package(Arrow CONFIG REQUIRED)
    find_package(Parquet CONFIG REQUIRED)
    target_compile_definitions(itch PUBLIC ITCH_WITH_ARROW)
    target_link_libraries(itch PUBLIC Arrow::arrow_shared Parquet::parquet_shared)
    message(STATUS "ITCH: Apache Arrow / Parquet export enabled")
endif()
add_library(itch::itch ALIAS itch)
if (NOT ANDROID)
    target_link_libraries(
        itch
        PRIVATE warnings::strict
    )
endif()

configure_visibility(itch)

# itch is a static library by default, but the pybind11 module (and any other
# shared-library consumer) links its object code into a .so, which requires
# position-independent code regardless of how itch itself is built.
set_target_properties(itch PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Step 2: Update Include Directories
target_include_directories(
    itch PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

)

# Step 3: Specify Output Destinations
install(
    TARGETS itch
    EXPORT ItchTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    TARGETS warnings
    EXPORT ItchTargets
)

# Step 4: Specify Header Destinations
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/itch
)

# Step 5: Export the Target Information
install(
    EXPORT ItchTargets
    FILE ItchTargets.cmake 
    NAMESPACE itch::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/itch
)

# Step 6: Creat the Main Package Configuration File
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/ItchConfig.cmake.in"
    "ItchConfig.cmake"
    INSTALL_DESTINATION 
    "${CMAKE_INSTALL_LIBDIR}/cmake/itch"
)
# Step 7: Creat a Package Version File
write_basic_package_version_file(
    "ItchConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Step 8: Install the Generated Package Files
install(
    FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/ItchConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ItchConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/itch"
)
