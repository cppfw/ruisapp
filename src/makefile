include prorab.mk
include prorab-license.mk
include prorab-linux.mk
include prorab-clang-format.mk
include prorab-install-dbgsrc.mk

# =====================================
# === wayland shell client protocol ===

ifeq ($(config), emsc)
else ifeq ($(os), linux)
    this_out_dir := out/

    this_name := xdg-shell-client-protocol

    this__wayland_shell_protocol_source := $(this_out_dir)$(this_name).c

    wayland_shell_protocol_header := $(this_out_dir)$(this_name).h

    this_srcs := $(this__wayland_shell_protocol_source)

    this_static_lib_only := true
    this_no_install := true

    $(eval $(prorab-build-lib))

    wayland_shell_protocol_lib := $(prorab_this_static_lib)

    define this__rules
        $(d)$(this__wayland_shell_protocol_source): /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
$(.RECIPEPREFIX)$(a)echo generate $$@
$(.RECIPEPREFIX)$(a)mkdir -p $$(dir $$@)
$(.RECIPEPREFIX)$(a)wayland-scanner private-code $$< $$@

        $(d)$(wayland_shell_protocol_header): /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
$(.RECIPEPREFIX)$(a)echo generate $$@
$(.RECIPEPREFIX)$(a)mkdir -p $$(dir $$@)
$(.RECIPEPREFIX)$(a)wayland-scanner client-header $$< $$@
    endef
    $(eval $(this__rules))

endif

# ===============
# === ruisapp ===
define ruisapp_rules
    $$(eval $$(prorab-clear-this-vars))

    $$(eval $$(call prorab-config, ../config))

    this_out_dir := $$(this_out_dir)$1$$(if $2,-$2)

    this_soname := $(shell cat $(d)soname.txt)

    this_srcs += $$(call prorab-src-dir, ruisapp)

    this_name := ruisapp-$1$(if $2,-$2)$$(this_dbg)

    ifeq ($2,sdl)
        this_ldlibs += -l SDL2

        this_cxxflags += -D RUISAPP_BACKEND_SDL

    else ifeq ($(os), linux)
        # use -isystem instead of -I to prevent clang-tidy follow the includes from these locations
        this_cxxflags += $$(patsubst -I%,-isystem%,$$(shell pkg-config --cflags gdk-3.0))
        this_ldlibs += $$(shell pkg-config --libs gdk-3.0)

        this_ldlibs += -l GLEW

        # TODO: add linking to dbg versions of all libs
        this_ldlibs += -l nitki
        this_ldlibs += -l opros

        ifeq ($2,wayland)
            $$(d)ruisapp/glue/glue.cpp: $$(d)$(wayland_shell_protocol_header)

            this_ldlibs += -l wayland-client
            this_ldlibs += -l wayland-egl
            this_ldlibs += -l wayland-cursor
            this_ldlibs += $(wayland_shell_protocol_lib)
            this_ldlibs += -l xkbcommon

            this_cxxflags += -isystem $(dir $(wayland_shell_protocol_header))
            this_cxxflags += -D RUISAPP_BACKEND_WAYLAND
        else ifeq ($2,xorg)
            this_ldlibs += -l X11
        else
        endif

    else ifeq ($(os), windows)
        this_ldlibs += -l gdi32
        this_ldlibs += -l opengl32
        this_ldlibs += -l glew32

    else ifeq ($(os), macosx)
        this_ldlibs += -l GLEW
        this_ldlibs += -framework Cocoa
        this_ldlibs += -framework OpenGL
        this_ldlibs += -l dl

        this_mm_obj := $$(d)$$(this_out_dir)obj_$$(this_name)/objc/ruisapp/glue/macosx/glue.mm.o

        define this_subrules
            $$(this_mm_obj): $$(d)ruisapp/glue/macosx/glue.mm
$(.RECIPEPREFIX)@echo compile $$$$<
$(.RECIPEPREFIX)$(a)mkdir -p $$$$(dir $$$$@)
$(.RECIPEPREFIX)$(a)$(this_cc) -ObjC++ -c -o "$$$$@" $$(this_cxxflags) $$(OBJCXXFLAGS) $$(CXXFLAGS) $$ $$$$<
        endef
        $$(eval $$(this_subrules))
    endif

    # TODO: add linking to dbg versions of all libs
    this_ldlibs += -l ruis$$(this_dbg)
    this_ldlibs += -l papki
    this_ldlibs += -l tml
    this_ldlibs += -l utki$$(this_dbg)

    this_cxxflags += -D RUISAPP_RENDER_$(shell echo $1 | tr '[:lower:]' '[:upper:]')
    this_ldlibs += -l ruis-render-$1$$(this_dbg)

    ifeq ($1,opengles)
        this_ldlibs += -l EGL
    endif

    $$(eval $$(prorab-build-lib))

    ifeq ($2,sdl)
    else ifeq ($(os), macosx)
        $$(prorab_this_staticlib): $$(this_mm_obj)
        $$(prorab_this_name): $$(this_mm_obj)
    
    else ifeq ($(os), linux)
        ifeq ($2,wayland)
            $$(prorab_this_staticlib): $(wayland_shell_protocol_lib)
            $$(prorab_this_name): $(wayland_shell_protocol_lib)
        endif
    endif
endef

ifeq ($(os), linux)
    ifneq ($(config), emsc)
        $(eval $(call ruisapp_rules,opengl,xorg))
        $(eval $(call ruisapp_rules,opengles,xorg))
        # $(eval $(call ruisapp_rules,opengl,wayland))
        $(eval $(call ruisapp_rules,opengles,wayland))
        $(eval $(call ruisapp_rules,opengl,sdl))
    endif

    $(eval $(call ruisapp_rules,opengles,sdl))

else
    $(eval $(call ruisapp_rules,opengl,))
endif

# clear variable
ruisapp_rules :=

# ===============================
# === license, format, dbgsrc ===
$(eval $(prorab-clear-this-vars))

$(eval $(call prorab-config, ../config))

this_license_file := ../LICENSE
this_src_dir := ruisapp
$(eval $(prorab-license))

$(eval $(prorab-clang-format))

this_name := ruisapp
this_soname := $(shell cat $(d)soname.txt)
$(eval $(prorab-install-dbgsrc))
