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

if(IOS)
    enable_language(OBJCXX)
    list(APPEND srcs ../../src/${name}/glue/ios/glue.mm)

    set(ruisapp_build_opengl_variant False)
    set(ruisapp_build_opengles_variant True)
elseif(APPLE)
    enable_language(OBJCXX)
    list(APPEND srcs ../../src/${name}/glue/macos/glue.mm)

    set(ruisapp_build_opengl_variant True)
    set(ruisapp_build_opengles_variant False)
else()
    set(ruisapp_build_opengl_variant True)
    set(ruisapp_build_opengles_variant True)
endif()

if(ruisapp_build_opengl_variant)
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
endif()

if(ruisapp_build_opengles_variant)
    myci_declare_library(${name}-opengles
        GUI
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

if(ruisapp_build_opengl_variant AND ruisapp_build_opengles_variant)
    myci_export(
        TARGETS
            ${name}-opengl
            ${name}-opengles
    )
elseif(ruisapp_build_opengl_variant)
    myci_export(
        TARGETS
            ${name}-opengl
    )
elseif(ruisapp_build_opengles_variant)
    myci_export(
        TARGETS
            ${name}-opengles
    )
endif()

set(app_srcs)
myci_add_source_files(app_srcs
    DIRECTORY
        ../../tests/app/src
    RECURSIVE
)

if(ruisapp_build_opengl_variant)
    myci_declare_application(${name}-opengl-test
        GUI
        SOURCES
            ${app_srcs}
        RESOURCE_DIRECTORY
            ../../tests/app/res
        DEPENDENCIES
            ruisapp::ruisapp-opengl
    )

    if(APPLE)
        # link to Cocoa framework
        target_link_libraries(${name}-opengl-test
            PRIVATE
                # Link the Cocoa framework directly using the -framework flag string.
                # The quotes are important so CMake passes it as a single linker argument.
                "-framework Cocoa"
        )

        set_target_properties(${name}-opengl-test PROPERTIES
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "io.github.cppfw.ruisapp-opengl-test"
        )
    endif()
endif()

if(ruisapp_build_opengles_variant)
    myci_declare_application(${name}-opengles-test
        GUI
        SOURCES
            ${app_srcs}
        RESOURCE_DIRECTORY
            ../../tests/app/res
        DEPENDENCIES
            ruisapp::ruisapp-opengles
    )

    if(APPLE)
        target_link_libraries(${name}-opengles-test
            PRIVATE
                # Link the iOS frameworks directly using the -framework flag string.
                # The quotes are important so CMake passes it as a single linker argument.
                "-framework Foundation"
                "-framework UIKit"
                "-framework OpenGLES" # TODO: is needed? ruis-render-opengles already links against it
                "-framework GLKit" # for GLKViewController
        )

        # TODO: is this needed?
        target_link_options(
            ${name}-opengles-test
            PRIVATE
                "-ObjC" # Use -ObjC (case sensitive) to pass the linker flag
        )

        set_target_properties(${name}-opengles-test PROPERTIES
            MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.cppfw.${name}-opengles-test"    
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "io.github.cppfw.${name}-opengles-test"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "948FQ58W9M"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        )
    endif()
endif()
