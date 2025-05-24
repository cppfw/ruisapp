cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

project(test)

find_package(utki CONFIG REQUIRED)
find_package(papki CONFIG REQUIRED)
find_package(rasterimage CONFIG REQUIRED)
find_package(r4 CONFIG REQUIRED)
find_package(tml CONFIG REQUIRED)
find_package(agg CONFIG REQUIRED)
find_package(veg CONFIG REQUIRED)
find_package(mikroxml CONFIG REQUIRED)
find_package(cssom CONFIG REQUIRED)
find_package(svgdom CONFIG REQUIRED)
find_package(svgren CONFIG REQUIRED)
find_package(opros CONFIG REQUIRED)
find_package(nitki CONFIG REQUIRED)
find_package(ruis CONFIG REQUIRED)
find_package(ruis-render-opengl CONFIG REQUIRED)

find_package(Freetype CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(GLEW REQUIRED)

if(LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GDK3 REQUIRED IMPORTED_TARGET "gdk-3.0")
    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET "x11")
    pkg_check_modules(EGL REQUIRED IMPORTED_TARGET "egl")
    pkg_check_modules(GLES2 REQUIRED IMPORTED_TARGET "glesv2")
elseif(WIN32)
    find_package(unofficial-angle CONFIG REQUIRED)
endif()

find_package(ruisapp CONFIG REQUIRED)

add_executable(test main.cpp)

target_link_libraries(test PRIVATE ruisapp::ruisapp-opengl)
