cmake_minimum_required(VERSION 3.25)

include(GNUInstallDirs)

add_executable(itch_tool main.cpp)
set_target_properties(itch_tool PROPERTIES OUTPUT_NAME "itch-tool")

target_link_libraries(
    itch_tool PRIVATE
    itch::itch
    warnings::strict
)

install(TARGETS itch_tool RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
