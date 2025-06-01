cmake_minimum_required(VERSION 3.25)

set(name ruisapp)
project(${name})

# !!! find_package must go after project() declaration !!!
# Otherwise VCPKG does not set the CMAKE_PREFIX_PATH to find packages.
find_package(myci CONFIG REQUIRED)

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype CONFIG REQUIRED)

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
    set(windows_deps)
elseif(WIN32)
    find_package(unofficial-angle CONFIG REQUIRED)
    set(linux_deps)
    set(windows_deps
        unofficial::angle::libGLESv2
        unofficial::angle::libEGL
    )
else()
    set(linux_deps)
    set(windows_deps)
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
    EXTERNAL_DEPENDENCIES
        JPEG::JPEG
        PNG::PNG
        GLEW::GLEW
        Freetype::Freetype
        ${linux_deps}
        ${windows_deps}
    NO_EXPORT
)

myci_declare_library(${name}-opengles
    SOURCES
        ${srcs}
    PUBLIC_INCLUDE_DIRECTORIES
        ../../src
    # header files are installed by ${name}-opengl target
    # INSTALL_INCLUDE_DIRECTORIES
    #     ../../src/${name}
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
        nitki
        ruis
        ruis-render-opengles
    EXTERNAL_DEPENDENCIES
        JPEG::JPEG
        PNG::PNG
        Freetype::Freetype
        ${linux_deps}
        ${windows_deps}
    NO_EXPORT
)

myci_export(
    TARGETS
        ${name}-opengl
        ${name}-opengles
)

set(app_srcs)
myci_add_source_files(app_srcs
    DIRECTORY
        ../../tests/app/src
    RECURSIVE
)

myci_declare_application(${name}-opengl-test
    GUI
    SOURCES
        ${app_srcs}
    RESOURCE_DIRECTORY
        ../../tests/app/res
    DEPENDENCIES
        ruis
        nitki
        ruisapp::ruisapp-opengl
)

myci_declare_application(${name}-opengles-test
    GUI
    SOURCES
        ${app_srcs}
    RESOURCE_DIRECTORY
        ../../tests/app/res
    DEPENDENCIES
        ruis
        nitki
        ruisapp::ruisapp-opengles
    EXTERNAL_DEPENDENCIES
        ${windows_deps}
)
