cmake_minimum_required(VERSION 3.25)

# The Python bindings are built only when ITCH_BUILD_PYTHON is ON. pybind11 is
# resolved through vcpkg or a system install.
# Force pybind11 onto the modern FindPython path; its default legacy
# FindPythonLibsNew.cmake wrapper calls the CMake-deprecated FindPythonInterp
# module, which recent CMake releases (4.x) no longer support, breaking
# configure in isolated build environments such as vcpkg's CI.
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

# The compiled extension is the private `_itchcpp` module imported by the pure
# Python `itchcpp` package, which re-exports it under the upstream-compatible
# `itchcpp.messages` / `itchcpp.parser` / `itchcpp.indicators` layout.
pybind11_add_module(itchcpp_python src/bindings.cpp)
set_target_properties(itchcpp_python PROPERTIES OUTPUT_NAME "_itchcpp")

target_link_libraries(itchcpp_python PRIVATE itch::itch)

# When built through scikit-build-core (pip install .) install the extension into
# the itchcpp package directory alongside the pure-Python modules.
if(DEFINED SKBUILD)
    install(TARGETS itchcpp_python LIBRARY DESTINATION itchcpp)
endif()
