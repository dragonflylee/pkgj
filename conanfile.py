from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain


class PkgjConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        if self.settings.os == "PSVita":
            self.requires("vitasqlite/0.0.2@blastrock/pkgj")
            self.requires("libvita2d/0.0.2@blastrock/pkgj")
            self.requires("imgui/1.89.4")
        else:
            self.requires("sqlite3/3.42.0")

        self.requires("boost/1.82.0")
        self.requires("libzip/1.9.2@blastrock/pkgj")
        self.requires("fmt/10.0.0")
        self.requires("cereal/1.3.2")

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.generate()

    def configure(self):
        self.options["boost"].header_only = True
        self.options["fmt"].fPIC = False
        self.options["fmt"].shared = False
        self.options["zlib"].fPIC = False
        self.options["zlib"].shared = False
        self.options["bzip2"].build_executable = False
        self.options["bzip2"].shared = False
        self.options["bzip2"].fPIC = False
        self.options["libzip"].tools = False
        self.options["libzip"].with_lzma = False
        self.options["libzip"].with_zstd = False
        self.options["libzip"].crypto = False
        self.options["libzip"].fPIC = False
        self.options["libzip"].shared = False
        self.options["imgui"].fPIC = False
        self.options["imgui"].shared = False

    def build(self):
        cmake = CMake(self)
        variables = {}
        if self.settings.os != "PSVita":
            variables["HOST_BUILD"] = True
        cmake.configure(variables)
        cmake.build()
