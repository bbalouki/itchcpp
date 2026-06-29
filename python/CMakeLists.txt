cmake_minimum_required(VERSION 3.25)

# The Python bindings are built only when ITCH_BUILD_PYTHON is ON. pybind11 is
# resolved through vcpkg or a system install.
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(itchcpp_python src/bindings.cpp)
set_target_properties(itchcpp_python PROPERTIES OUTPUT_NAME "itchcpp")

target_link_libraries(itchcpp_python PRIVATE itch::itch)

# When built through scikit-build-core (pip install .) install into the wheel.
if(DEFINED SKBUILD)
    install(TARGETS itchcpp_python LIBRARY DESTINATION .)
endif()
