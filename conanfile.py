from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class ClionCmakeOpenCVSample(ConanFile):
    name = "clion_cmake_opencv_sample"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        # OpenCV is intentionally NOT listed here - it comes from the
        # pre-built distribution at C:/opencv (see CMakeLists.txt).
        # Add any other non-OpenCV dependency here; ConanBootstrap.cmake
        # resolves it automatically on the next CMake configure.
        self.requires("fmt/11.0.2")

    def generate(self):
        CMakeDeps(self).generate()
        CMakeToolchain(self).generate()