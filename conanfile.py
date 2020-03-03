from conans import ConanFile, tools, CMake


class mediagraphConan(ConanFile):
    name = "mediagraph"
    license = "Custom"
    url = "https://github.com/jpilet/mediagraph"
    description = "A multi-threaded media pipelining library"
    author = "Julien Pilet <julien.pilet@opticode.ch>"
    topics = ("conan", "pipeline", "media", "multi-threading")
    
    exports = ("LICENSE.md", "README.md")
    exports_sources = ("*")
    
    generators = "cmake"
    settings = "os", "compiler", "build_type", "arch"
    options = {"fPIC": [True, False]}
    default_options = {"fPIC": True}

    def requirements(self):
        if self.user and self.channel:
            # If the recipe is using them, I want to consume my fork.
            self.requires("civetweb/[>=2.0.0-0]@%s/%s" % (self.user, self.channel))
        else:
            # otherwise, I'll consume the community one, if exists
            self.requires("civetweb/[>=2.0.0-0]")

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["CIVETWEB_LIB"] = "CONAN_PKG::civetweb"
        cmake.configure()
        return cmake

    def build(self):
        # conan will define what standard is used and set CMake variables accordingly
        tools.replace_in_file(file_path="CMakeLists.txt",
                              search="set (CMAKE_CXX_STANDARD 11)",
                              replace="""
                                 include(conanbuildinfo.cmake)
                                 conan_basic_setup(TARGETS)""")

        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include/mediagraph/", keep_path=True)
        self.copy("*.a", dst="lib/", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.os == "Linux":
            self.cpp_info.system_libs.extend(["pthread"])
