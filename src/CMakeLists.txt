cmake_minimum_required(VERSION 3.20)

include(GNUInstallDirs)

# Step 1: Add targets and properties
add_library(itch_parser parser.cpp messages.cpp)
add_library(itch::parser ALIAS itch_parser)
target_link_libraries(
    itch_parser
    PRIVATE warnings::strict
)

configure_visibility(itch_parser)

# Step 2: Update Include Directories
target_include_directories(
    itch_parser PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/itch>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/itch>

)

# Step 3: Specify Output Destinations
install(
    TARGETS itch_parser
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
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/itch/
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
