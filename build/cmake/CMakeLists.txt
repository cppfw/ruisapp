cmake_minimum_required(VERSION 3.25)

set(name ruisapp)
project(${name})

# !!! find_package must go after project() declaration !!!
# Otherwise VCPKG does not set the CMAKE_PREFIX_PATH to find packages.
find_package(myci CONFIG REQUIRED)

set(srcs)
myci_add_source_files(srcs
    DIRECTORY
        ../../src/${name}
    RECURSIVE
)

enable_language(OBJCXX)

if(APPLE)
    list(APPEND srcs ../../src/${name}/glue/macos/glue.mm)
endif()

myci_declare_library(${name}-opengl
    SOURCES
        ${srcs}
    PUBLIC_INCLUDE_DIRECTORIES
        ../../src
    INSTALL_INCLUDE_DIRECTORIES
        ../../src/${name}
    PREPROCESSOR_DEFINITIONS
        "RUISAPP_RENDER_OPENGL"
    DEPENDENCIES
        nitki
        ruis
        ruis-render-opengl
    LINUX_ONLY_DEPENDENCIES
        PkgConfig::gdk-3.0
        PkgConfig::x11
    NO_EXPORT
)

# no opengles build on MACOS for now
if(NOT APPLE)
    myci_declare_library(${name}-opengles
        SOURCES
            ${srcs}
        PUBLIC_INCLUDE_DIRECTORIES
            ../../src
        # header files are installed by ${name}-opengl target
        # INSTALL_INCLUDE_DIRECTORIES
        #     ../../src/${name}
        PREPROCESSOR_DEFINITIONS
            "RUISAPP_RENDER_OPENGLES"
        DEPENDENCIES
            nitki
            ruis
            ruis-render-opengles
        LINUX_ONLY_DEPENDENCIES
            PkgConfig::gdk-3.0
            PkgConfig::x11
            PkgConfig::egl
        WINDOWS_ONLY_DEPENDENCIES
            unofficial-angle/unofficial::angle::libEGL
        NO_EXPORT
    )
endif()

if(APPLE)
    myci_export(
        TARGETS
            ${name}-opengl
    )
else()
    myci_export(
        TARGETS
            ${name}-opengl
            ${name}-opengles
    )
endif()

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
        ruisapp::ruisapp-opengl
)

# link to Cocoa framework on macos
if(APPLE)
    target_link_libraries(${name}-opengl-test
        PRIVATE
            # Link the Cocoa framework directly using the -framework flag string.
            # The quotes are important so CMake passes it as a single linker argument.
            "-framework Cocoa"
        )
endif()

# no opengles build on MACOS for now
if(NOT APPLE)
    myci_declare_application(${name}-opengles-test
        GUI
        SOURCES
            ${app_srcs}
        RESOURCE_DIRECTORY
            ../../tests/app/res
        DEPENDENCIES
            ruisapp::ruisapp-opengles
    )
endif()
