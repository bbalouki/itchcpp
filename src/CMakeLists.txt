cmake_minimum_required(VERSION 3.20)

include(GNUInstallDirs)

# Step 1: Add targets and properties
add_library(itch parser.cpp messages.cpp order_book.cpp)
add_library(itch::itch ALIAS itch)
if (NOT ANDROID)
    target_link_libraries(
        itch
        PRIVATE warnings::strict
    )
endif()

configure_visibility(itch)

# Step 2: Update Include Directories
target_include_directories(
    itch PUBLIC
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
    FILES
        ${PROJECT_SOURCE_DIR}/include/itch/indicators.hpp
        ${PROJECT_SOURCE_DIR}/include/itch/messages.hpp
        ${PROJECT_SOURCE_DIR}/include/itch/order_book.hpp
        ${PROJECT_SOURCE_DIR}/include/itch/parser.hpp
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
