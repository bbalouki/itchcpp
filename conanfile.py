"""Conan recipe for ITCHCPP.

Packages the header-only-friendly ITCH 5.0 library (parser, transport decoders,
book engine, analytics, encoder, replay) so downstream projects can depend on it
through Conan. Tests, benchmarks, tools, and the optional Arrow/Python features are
not part of the consumed package; enable them from a source build instead.
"""

import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy, load


class ItchConan(ConanFile):
    name = "itchcpp"
    license = "MIT"
    url = "https://github.com/bbalouki/itchcpp"
    description = (
        "High-performance C++20 NASDAQ TotalView-ITCH 5.0 parser, order-book "
        "engine, analytics, and feed-ingestion library."
    )
    topics = ("nasdaq", "itch", "market-data", "order-book", "hft", "finance")
    settings = "os", "compiler", "build_type", "arch"
    options = {"with_arrow": [True, False]}
    default_options = {"with_arrow": False}
    exports_sources = (
        "CMakeLists.txt",
        "VERSION.txt",
        "cmake/*",
        "src/*",
        "include/*",
    )

    def set_version(self):
        self.version = load(
            self, os.path.join(self.recipe_folder, "VERSION.txt")
        ).strip()

    def requirements(self):
        if self.options.with_arrow:
            self.requires("arrow/15.0.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.variables["ITCH_WITH_ARROW"] = self.options.with_arrow
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(
            self,
            "LICENSE",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
        )

    def package_info(self):
        self.cpp_info.libs = ["itch"]
        self.cpp_info.set_property("cmake_file_name", "itch")
        self.cpp_info.set_property("cmake_target_name", "itch::itch")
