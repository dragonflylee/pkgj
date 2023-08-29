#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

from conan import ConanFile
from conan.tools import files


class Vita2dConan(ConanFile):
    name = "libvita2d"
    version = "0.0.2"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "temp-support.patch"
    user = "blastrock"
    channel = "pkgj"

    def export_sources(self):
        files.export_conandata_patches(self)

    def source(self):
        files.get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def build(self):
        files.apply_conandata_patches(self)
        self.run("make -C libvita2d")

    def package(self):
        self.run("make -C libvita2d install")

    def package_info(self):
        self.cpp_info.libs = ["libvita2d"]
