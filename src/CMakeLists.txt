cmake_minimum_required(VERSION 3.20)


# add_library(messageslib messages.cpp)
# set_cxx_std(messageslib 23)
# target_link_libraries(messageslib warnings::strict)
# add_library(parserlib parser.cpp)
# set_cxx_std(parserlib 23)
# target_link_libraries(parserlib warnings::strict messageslib)

# add_library(itch::parser ALIAS parserlib)
# add_library(itch::messages ALIAS messageslib)
# target_include_directories(parserlib PUBLIC 
#     ${PROJECT_SOURCE_DIR}/include/itch
# )
# target_include_directories(messageslib PUBLIC
#     ${PROJECT_SOURCE_DIR}/include/itch
# )


include(GNUInstallDirs)

add_library(messages messages.cpp)
set_cxx_std(messages 23)
target_link_libraries(messages warnings::strict)
add_library(parser parser.cpp)
target_link_libraries(parser warnings::strict messages)
add_library(itch::parser ALIAS parser)

set_target_properties(
    parser PROPERTIES
    OUTPUT_NAME parser 
    EXPORT_NAME Lib 
)

# Step 2: Updating Include Directories
target_include_directories(
    parser PUBLIC 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/itch>

)

# Step 3: Specifying Output Destinations
install(
    TARGETS parser
    EXPORT ItchTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Step 4: Specifying Header Destinations
file(
    GLOB PARSER_ROOT_HEADERS 
    "${PROJECT_SOURCE_DIR}/include/*.h"
)
install(
    FILES ${PARSER_ROOT_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/itch
)

# Step 5: Exporting the Target Information
install(
    EXPORT ItchTargets
    FILE ItchTargets.cmake 
    NAMESPACE itch::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/itch
)

# Step 6: Creating the Main Package Configuration File
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "cmake/ItchConfig.cmake.in"
    "ItchConfig.cmake"
    INSTALL_DESTINATION 
    "${CMAKE_INSTALL_LIBDIR}/cmake/itch"
)
# Step 7: Creating a Package Version File
write_basic_package_version_file(
    "ItchConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Step 8: Installing the Generated Package Files
install(
    FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/ItchConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ItchConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/itch"
)
