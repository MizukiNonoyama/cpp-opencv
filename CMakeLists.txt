cmake_minimum_required(VERSION 3.24)
project(clion_cmake CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# OpenCV: pre-built distribution at C:/opencv, NOT managed by Conan.
# BYPASS_PROVIDER skips the Conan dependency provider below and does a
# normal CMake config-mode search using OpenCV_DIR.
# ---------------------------------------------------------------------------
set(OpenCV_DIR "C:/opencv/build" CACHE PATH "Path to the OpenCV build directory")
find_package(OpenCV REQUIRED BYPASS_PROVIDER)

# ---------------------------------------------------------------------------
# Everything else: resolved automatically by Conan the first time it's
# needed (see conanfile.py + cmake/ConanBootstrap.cmake). To pull in
# another non-OpenCV dependency, add it to conanfile.py's requirements()
# and just find_package() it here like fmt below.
# ---------------------------------------------------------------------------
find_package(fmt REQUIRED)

add_executable(clion_cmake main.cpp)
target_link_libraries(clion_cmake PRIVATE ${OpenCV_LIBS} fmt::fmt)
target_include_directories(clion_cmake PRIVATE ${OpenCV_INCLUDE_DIRS})

# Copy OpenCV's runtime DLLs next to the exe so it runs without editing PATH.
if(WIN32)
    file(GLOB OPENCV_DLLS "${OpenCV_DIR}/x64/vc16/bin/*.dll")
    add_custom_command(TARGET clion_cmake POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OPENCV_DLLS} $<TARGET_FILE_DIR:clion_cmake>
        COMMENT "Copying OpenCV runtime DLLs next to the executable"
    )
endif()