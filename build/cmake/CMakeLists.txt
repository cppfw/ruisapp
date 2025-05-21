cmake_minimum_required(VERSION 3.25)

set(name ruisapp)
project(${name})

# !!! find_package must go after project() declaration !!!
# Otherwise VCPKG does not set the CMAKE_PREFIX_PATH to find packages.
find_package(myci CONFIG REQUIRED)

find_package(JPEG REQUIRED)
find_package(GLEW REQUIRED)

if(LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GDK3 REQUIRED IMPORTED_TARGET "gdk-3.0")
    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET "x11")
    pkg_check_modules(EGL REQUIRED IMPORTED_TARGET "egl")
    pkg_check_modules(GLES2 REQUIRED IMPORTED_TARGET "glesv2")
    set(linux_deps
        PkgConfig::GDK3
        PkgConfig::X11
        PkgConfig::EGL
        PkgConfig::GLES2
    )
else()
    set(linux_deps)
endif()

set(srcs)
myci_add_source_files(srcs
    DIRECTORY
        ../../src/${name}
    RECURSIVE
)

myci_declare_library(${name}-opengl
    SOURCES
        ${srcs}
    PUBLIC_INCLUDE_DIRECTORIES
        ../../src
    INSTALL_INCLUDE_DIRECTORIES
        ../../src/${name}
    PUBLIC_COMPILE_DEFINITIONS
        "RUISAPP_RENDER_OPENGL"
    DEPENDENCIES
        rasterimage
        r4
        papki
        tml
        utki
        agg
        veg
        mikroxml
        cssom
        svgdom
        svgren
        opros
        nitki
        ruis
        ruis-render-opengl
        Freetype
    EXTERNAL_DEPENDENCIES
        JPEG::JPEG
        GLEW::GLEW
        ${linux_deps}
)

myci_declare_library(${name}-opengles
    SOURCES
        ${srcs}
    PUBLIC_INCLUDE_DIRECTORIES
        ../../src
    INSTALL_INCLUDE_DIRECTORIES
        ../../src/${name}
    PUBLIC_COMPILE_DEFINITIONS
        "RUISAPP_RENDER_OPENGLES"
    DEPENDENCIES
        rasterimage
        r4
        papki
        tml
        utki
        agg
        veg
        mikroxml
        cssom
        svgdom
        svgren
        ruis
        ruis-render-opengles
        Freetype
    EXTERNAL_DEPENDENCIES
        JPEG::JPEG
        ${linux_deps}
)

set(app_srcs)
myci_add_source_files(app_srcs
    DIRECTORY
        ../../tests/app/src
    RECURSIVE
)

set(app_rsrs)
# TODO:
# add_resource_directory(app_rsrs
#     "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/app/res"
#     )

myci_declare_application(${name}-opengl-test
    SOURCES
        ${app_srcs}
        ${app_rsrs}
    DEPENDENCIES
        ruis
        nitki
    LINK_LIBRARIES
        ruisapp-opengl
)

myci_declare_application(${name}-opengles-test
    SOURCES
        ${app_srcs}
        ${app_rsrs}
    DEPENDENCIES
        ruis
        nitki
    LINK_LIBRARIES
        ruisapp-opengles
)
