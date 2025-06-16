# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

set(name ruisapp_application) # this has to be 'ruisapp_application' so that RuisappActivity loads that named .so
project(${name})

file(GLOB_RECURSE srcs "../../../tests/app/*.cpp")

include(${ANDROID_GRADLE_NATIVE_BUNDLE_PLUGIN_MK})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

link_directories( ../ruisapp/build/intermediates/cmake/debug/obj/${ANDROID_ABI} )

add_library(
        # Specifies the name of the library.
        ${name}

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${srcs}
    )

target_link_libraries(
        ${name}
        ruisapp android log GLESv2 EGL z ${ANDROID_GRADLE_NATIVE_MODULES}
    )
