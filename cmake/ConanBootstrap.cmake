# cmake/ConanBootstrap.cmake
#
# Semi-automatic Conan dependency resolution via CMake's Dependency
# Provider hook (requires CMake >= 3.24).
#
# Any find_package(<pkg> ...) call in this project transparently triggers
# `conan install` the first time it's needed (if the generated toolchain
# isn't there yet) and is then satisfied from Conan's CMakeDeps output.
# OpenCV is excluded via BYPASS_PROVIDER in the top-level CMakeLists.txt
# since it comes from the pre-built distribution at C:/opencv, not Conan.
#
# "Semi-automatic" because you still need Conan installed and this file
# wired in once via CMAKE_PROJECT_TOP_LEVEL_INCLUDES; after that, adding a
# dependency is just: add it to conanfile.py, reconfigure, done.
#
# Enable with:
#   cmake -S . -B build -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=cmake/ConanBootstrap.cmake
# or, in CLion: Settings > Build, Execution, Deployment > CMake > CMake options:
#   -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=cmake/ConanBootstrap.cmake

cmake_minimum_required(VERSION 3.24)

function(conan_bootstrap_provide_package method package_name)
    if(NOT method STREQUAL "FIND_PACKAGE")
        return()
    endif()

    set(generators_dir "${CMAKE_BINARY_DIR}/conan")
    set(toolchain_file "${generators_dir}/conan_toolchain.cmake")

    if(NOT EXISTS "${toolchain_file}")
        # GUI-launched processes (CLion, Visual Studio, ...) often don't see
        # PATH entries added after they were started - e.g. the Scripts dir
        # pip installs console scripts into. Fall back to searching common
        # per-user Python install locations directly so this doesn't depend
        # on the caller's PATH or require restarting the IDE.
        file(GLOB _conan_bootstrap_py_dirs
            "$ENV{LOCALAPPDATA}/Programs/Python/Python3*"
            "$ENV{APPDATA}/Python/Python3*"
        )
        set(_conan_bootstrap_hints)
        foreach(_dir ${_conan_bootstrap_py_dirs})
            list(APPEND _conan_bootstrap_hints "${_dir}/Scripts")
        endforeach()

        find_program(CONAN_COMMAND conan HINTS ${_conan_bootstrap_hints} REQUIRED)

        if(CMAKE_BUILD_TYPE)
            set(build_type "${CMAKE_BUILD_TYPE}")
        else()
            set(build_type "Release")
        endif()

        message(STATUS "[ConanBootstrap] '${package_name}' not resolved yet -> running "
                        "'conan install' (build_type=${build_type})")

        execute_process(
            COMMAND "${CONAN_COMMAND}" install "${CMAKE_SOURCE_DIR}"
                    "--output-folder=${generators_dir}"
                    "--build=missing"
                    "-s" "build_type=${build_type}"
                    # Without this, Conan's CMakeToolchain assumes a Visual
                    # Studio generator whenever compiler=msvc and force-sets
                    # CMAKE_GENERATOR_PLATFORM=x64 into the cache - which
                    # Ninja/Makefiles reject on the *next* reconfigure (the
                    # first one doesn't validate it, so the breakage only
                    # shows up later, e.g. on the following `cmake --build`).
                    "-c" "tools.cmake.cmaketoolchain:generator=${CMAKE_GENERATOR}"
            RESULT_VARIABLE conan_install_result
        )
        if(NOT conan_install_result EQUAL 0)
            message(FATAL_ERROR "[ConanBootstrap] 'conan install' failed (exit code ${conan_install_result})")
        endif()
    endif()

    include("${toolchain_file}")
    find_package(${package_name} ${ARGN} BYPASS_PROVIDER)
endfunction()

cmake_language(SET_DEPENDENCY_PROVIDER conan_bootstrap_provide_package SUPPORTED_METHODS FIND_PACKAGE)