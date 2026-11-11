cmake_minimum_required(VERSION 3.30)

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

    set(ruisapp_opengl_install_include_directory) # opengl variant will not be built, no headers to install
    set(ruisapp_opengles_install_include_directory ../../src/${name})
elseif(APPLE)
    enable_language(OBJCXX)
    list(APPEND srcs ../../src/${name}/glue/macos/glue.mm)

    set(ruisapp_build_opengl_variant True)
    set(ruisapp_build_opengles_variant False)

    set(ruisapp_opengl_install_include_directory ../../src/${name})
    set(ruisapp_opengles_install_include_directory) # opengles variant will not be built, no headers to install
elseif(LINUX)
    set(ruisapp_build_opengl_variant True)
    set(ruisapp_build_opengles_variant True)

    # On Linux the library is built as ruisapp-<renderer>-<backend> variants,
    # the headers are installed by the opengl-xorg variant, see below.
    set(ruisapp_opengl_install_include_directory)
    set(ruisapp_opengles_install_include_directory)
else()
    set(ruisapp_build_opengl_variant True)
    set(ruisapp_build_opengles_variant True)

    set(ruisapp_opengl_install_include_directory ../../src/${name})
    set(ruisapp_opengles_install_include_directory) # opengl variant is also built and will install the headers
endif()

# ====================
# === library targets ===

if(LINUX)
    # On Linux the library is built as ruisapp-<renderer>-<backend> variants,
    # the same naming convention that was used by the old makefile based build system:
    # <renderer> = opengl, opengles
    # <backend> = xorg, wayland, sdl

    # ---- xdg-shell protocol generation (needed by the wayland backend) ----

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED IMPORTED_TARGET wayland-protocols)

    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

    pkg_get_variable(wayland_protocols_pkgdatadir wayland-protocols pkgdatadir)
    set(wayland_xdg_shell_protocol_xml "${wayland_protocols_pkgdatadir}/stable/xdg-shell/xdg-shell.xml")
    if(NOT EXISTS ${wayland_xdg_shell_protocol_xml})
        message(FATAL_ERROR "xdg-shell protocol xml not found: ${wayland_xdg_shell_protocol_xml}")
    endif()

    set(wayland_protocol_dir "${CMAKE_CURRENT_BINARY_DIR}/wayland-protocols")
    file(MAKE_DIRECTORY ${wayland_protocol_dir})

    set(wayland_xdg_shell_protocol_header "${wayland_protocol_dir}/xdg-shell-client-protocol.h")
    set(wayland_xdg_shell_protocol_source "${wayland_protocol_dir}/xdg-shell-client-protocol.c")

    add_custom_command(
        OUTPUT
            "${wayland_xdg_shell_protocol_header}"
            "${wayland_xdg_shell_protocol_source}"
        COMMAND
            "${WAYLAND_SCANNER}" client-header "${wayland_xdg_shell_protocol_xml}" "${wayland_xdg_shell_protocol_header}"
        COMMAND
            "${WAYLAND_SCANNER}" private-code "${wayland_xdg_shell_protocol_xml}" "${wayland_xdg_shell_protocol_source}"
        DEPENDS
            "${wayland_xdg_shell_protocol_xml}"
        COMMENT
            "Generating the xdg-shell client protocol"
        VERBATIM
    )

    # Declare a ruisapp-<renderer>-<backend> library variant.
    # renderer: opengl, opengles
    # backend: xorg, wayland, sdl
    # INSTALL_HEADERS: install the public headers, only one variant should install them.
    function(ruisapp_declare_variant renderer backend)
        set(options INSTALL_HEADERS)
        cmake_parse_arguments(arg "${options}" "" "" ${ARGN})

        string(TOUPPER "${renderer}" renderer_upper)
        string(TOUPPER "${backend}" backend_upper)

        set(backend_deps PkgConfig::gtk4)
        if(backend STREQUAL "xorg")
            list(APPEND backend_deps PkgConfig::x11)
        elseif(backend STREQUAL "wayland")
            list(APPEND backend_deps
                PkgConfig::wayland-client
                PkgConfig::wayland-egl
                PkgConfig::wayland-cursor
                PkgConfig::xkbcommon
            )
        elseif(backend STREQUAL "sdl")
            # SDL2 is found via vcpkg (the "sdl2" manifest dependency), myci links it as SDL2::SDL2
            list(APPEND backend_deps SDL2)
        else()
            message(FATAL_ERROR "ruisapp_declare_variant(): unknown backend: ${backend}")
        endif()

        # the opengles renderer always needs EGL, and the wayland backend always uses EGL
        # (for both renderers) to create the GL context and surface, so it needs EGL too
        if(renderer STREQUAL "opengles" OR backend STREQUAL "wayland")
            list(APPEND backend_deps PkgConfig::egl)
        endif()

        set(private_include_directories)
        if(backend STREQUAL "wayland")
            # the wayland backend uses the generated xdg-shell client protocol header
            set(private_include_directories ${wayland_protocol_dir})
        endif()

        set(install_include_directories)
        if(arg_INSTALL_HEADERS)
            set(install_include_directories ../../src/${name})
        endif()

        myci_declare_library(${name}-${renderer}-${backend}
            SOURCES
                ${srcs}
            PUBLIC_INCLUDE_DIRECTORIES
                ../../src
            INSTALL_INCLUDE_DIRECTORIES
                ${install_include_directories}
            PREPROCESSOR_DEFINITIONS
                "RUISAPP_RENDER_${renderer_upper}"
                "RUISAPP_BACKEND_${backend_upper}"
            DEPENDENCIES
                nitki
                ruis
                ruis-render-${renderer}
                opros
                setka
                tml
            LINUX_ONLY_DEPENDENCIES
                ${backend_deps}
            PRIVATE_INCLUDE_DIRECTORIES
                ${private_include_directories}
            NO_EXPORT
        )

        if(backend STREQUAL "wayland")
            # the generated xdg-shell client protocol source is added after the library
            # declaration, so that it is not picked up by the format targets
            target_sources(${name}-${renderer}-${backend}
                PRIVATE
                    ${wayland_xdg_shell_protocol_source}
            )
        endif()
    endfunction()

    ruisapp_declare_variant(opengl xorg INSTALL_HEADERS)
    ruisapp_declare_variant(opengles xorg)
    ruisapp_declare_variant(opengl wayland)
    ruisapp_declare_variant(opengles wayland)
    ruisapp_declare_variant(opengl sdl)
    ruisapp_declare_variant(opengles sdl)

    # Compatibility aliases: for now ruisapp-opengl / ruisapp-opengles refer to the xorg variants.
    # Note: the namespaced alias refers to the plain (non-namespaced) target, the same
    # convention that myci_declare_library() uses for the ${PROJECT_NAME}::<name> alias.
    add_library(${name}-opengl ALIAS ${name}-opengl-xorg)
    add_library(${PROJECT_NAME}::${name}-opengl ALIAS ${name}-opengl-xorg)
    add_library(${name}-opengles ALIAS ${name}-opengles-xorg)
    add_library(${PROJECT_NAME}::${name}-opengles ALIAS ${name}-opengles-xorg)

else()
    if(ruisapp_build_opengl_variant)
        myci_declare_library(${name}-opengl
            SOURCES
                ${srcs}
            PUBLIC_INCLUDE_DIRECTORIES
                ../../src
            INSTALL_INCLUDE_DIRECTORIES
                ${ruisapp_opengl_install_include_directory}
            PREPROCESSOR_DEFINITIONS
                "RUISAPP_RENDER_OPENGL"
            DEPENDENCIES
                nitki
                ruis
                ruis-render-opengl
                opros
                setka
                tml
            LINUX_ONLY_DEPENDENCIES
                PkgConfig::gtk4
                PkgConfig::x11
            NO_EXPORT
        )

        if(APPLE)
            # link to Cocoa framework
            target_link_libraries(${name}-opengl
                PRIVATE
                    # Link the Cocoa framework directly using the -framework flag string.
                    # The quotes are important so CMake passes it as a single linker argument.
                    "-framework Cocoa"
            )

            # TODO: Define the GL_SILENCE_DEPRECATION macro to silence OpenGL deprecation warnings on Apple platforms. Need to move to Metal eventually.
            target_compile_definitions(${name}-opengl PRIVATE GL_SILENCE_DEPRECATION)

            # TODO: some stuff (e.g. NSOpenGLContext) is deprecated on MacOS, so suppress the warnings for now. Need to use new APIs eventually.
            target_compile_options(${name}-opengl PRIVATE -Wno-deprecated-declarations)
        endif()
    endif()

    if(ruisapp_build_opengles_variant)
        myci_declare_library(${name}-opengles
            SOURCES
                ${srcs}
            PUBLIC_INCLUDE_DIRECTORIES
                ../../src
            INSTALL_INCLUDE_DIRECTORIES
                ${ruisapp_opengles_install_include_directory}
            PREPROCESSOR_DEFINITIONS
                "RUISAPP_RENDER_OPENGLES"
            DEPENDENCIES
                nitki
                ruis
                ruis-render-opengles
                opros
                setka
                tml
            LINUX_ONLY_DEPENDENCIES
                PkgConfig::gtk4
                PkgConfig::x11
                PkgConfig::egl
            WINDOWS_ONLY_DEPENDENCIES
                unofficial-angle/unofficial::angle::libEGL
            NO_EXPORT
        )

        if(IOS)
            target_link_libraries(${name}-opengles
                PRIVATE
                    # Link the iOS frameworks directly using the -framework flag string.
                    # The quotes are important so CMake passes it as a single linker argument.
                    "-framework Foundation"
                    "-framework UIKit"
                    "-framework GLKit" # for GLKViewController
            )

            # TODO: Define the GLES_SILENCE_DEPRECATION macro to silence OpenGL ES deprecation warnings on Apple platforms. Need to move to Metal eventually.
            target_compile_definitions(${name}-opengles PRIVATE GLES_SILENCE_DEPRECATION)

            # TODO: some stuff (e.g. UI_USER_INTERFACE_IDIOM) is deprecated on iOS, so suppress the warnings for now. Need to use new APIs eventually.
            target_compile_options(${name}-opengles PRIVATE -Wno-deprecated-declarations)
        endif()
    endif()
endif()

# ====================
# === export ===

if(LINUX)
    myci_export(
        TARGETS
            ${name}-opengl-xorg
            ${name}-opengles-xorg
            ${name}-opengl-wayland
            ${name}-opengles-wayland
            ${name}-opengl-sdl
            ${name}-opengles-sdl
    )
elseif(ruisapp_build_opengl_variant AND ruisapp_build_opengles_variant)
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

# =================
# === test apps ===

set(app_srcs)
myci_add_source_files(app_srcs
    DIRECTORY
        ../../tests/app/src
    RECURSIVE
)
# Declare a GUI test app for the given ruisapp-<renderer>-<backend> library variant.
function(ruisapp_declare_test_app renderer backend)
    set(test_app_name "${name}-${renderer}-${backend}-test")
    myci_declare_application(${test_app_name}
        GUI
        SOURCES
            ${app_srcs}
        RESOURCE_DIRECTORY
            ../../tests/app/res
        DEPENDENCIES
            ${PROJECT_NAME}::${name}-${renderer}-${backend}
    )
endfunction()

if(LINUX)
    # On Linux there is a test app for every ruisapp-<renderer>-<backend> variant,
    # named <library-name>-test.
    foreach(renderer opengl opengles)
        foreach(backend xorg wayland sdl)
            ruisapp_declare_test_app(${renderer} ${backend})
        endforeach()
    endforeach()
else()
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
            set_target_properties(${name}-opengles-test PROPERTIES
                MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.cppfw.${name}-opengles-test"    
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "io.github.cppfw.${name}-opengles-test"
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "948FQ58W9M"
                XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
            )
        endif()
    endif()
endif()
