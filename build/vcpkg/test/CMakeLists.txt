cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

project(test)

if(LINUX)
    set(win32)
elseif(WIN32)
    set(win32 WIN32)
endif()

find_package(ruisapp CONFIG REQUIRED)

add_executable(test ${win32} main.cpp)

if(IOS)
    target_link_libraries(test PRIVATE ruisapp::ruisapp-opengles)
else()
    target_link_libraries(test PRIVATE ruisapp::ruisapp-opengl)
endif()
