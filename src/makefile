include prorab.mk
include prorab-license.mk
include prorab-linux.mk
include prorab-clang-format.mk

ifeq ($(os), linux)
    wayland_shell_protocol_source := out/xdg-shell-client-protocol

    define this__rules
        $(d)$(wayland_shell_protocol_source).c: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
$(.RECIPEPREFIX)$(a)echo generate $$@
$(.RECIPEPREFIX)$(a)mkdir -p $(dir $$@)
$(.RECIPEPREFIX)$(a)wayland-scanner private-code $$< $$@

        $(d)$(wayland_shell_protocol_source).h: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
$(.RECIPEPREFIX)$(a)echo generate $$@
$(.RECIPEPREFIX)$(a)mkdir -p $(dir $$@)
$(.RECIPEPREFIX)$(a)wayland-scanner client-header $$< $$@
    endef
    $(eval $(this__rules))
endif

$(eval $(prorab-clear-this-vars))

define ruisapp_rules

    $$(eval $$(call prorab-config, ../config))

    this_out_dir := $$(this_out_dir)$1$$(if $2,-$2)

    this_soname := $(shell cat $(d)soname.txt)

    this_srcs += $$(call prorab-src-dir, ruisapp)

    this_name := ruisapp-$1$(if $2,-$2)

    ifeq ($(os), linux)
        # use -isystem instead of -I to prevent clang-tidy follow the includes from these locations
        this_cxxflags += $$(patsubst -I%,-isystem%,$$(shell pkg-config --cflags gdk-3.0))
        this_ldlibs += $$(shell pkg-config --libs gdk-3.0)

        this_ldlibs += -lGLEW -lnitki -lopros

        ifeq ($2,wayland)
            this_srcs += $(wayland_shell_protocol_source).c
            $$(d)ruisapp/glue/glue.cpp: $$(d)$(wayland_shell_protocol_source).h

            this_ldlibs += -lwayland-client
            this_cxxflags += -D RUISAPP_WAYLAND
        else
            this_ldlibs += -lX11
        endif
    else ifeq ($(os), windows)
        this_ldlibs += -lgdi32 -lopengl32 -lglew32
    else ifeq ($(os), macosx)
        this_ldlibs += -lGLEW
        this_ldlibs += -framework Cocoa -framework OpenGL -ldl
    endif

    this_ldlibs += -lruis -lpapki -ltml -lutki

    ifeq ($(os), macosx)
        this_mm_obj := $$(d)$$(this_out_dir)obj_$$(this_name)/objc/ruisapp/glue/macosx/glue.mm.o

        define this_subrules
            $$(this_mm_obj): $$(d)ruisapp/glue/macosx/glue.mm
$(.RECIPEPREFIX)@echo compile $$$$<
$(.RECIPEPREFIX)$(a)mkdir -p $$$$(dir $$$$@)
$(.RECIPEPREFIX)$(a)$(this_cc) -ObjC++ -c -o "$$$$@" $$(this_cxxflags) $$(OBJCXXFLAGS) $$ $$$$<
        endef
        $$(eval $$(this_subrules))
    endif

    this_cxxflags += -D RUISAPP_RENDER_$(shell echo $1 | tr '[:lower:]' '[:upper:]')
    this_ldlibs += -lruis-render-$1

    ifeq ($1,opengles)
        this_ldlibs += -lEGL
    endif

    $$(eval $$(prorab-build-lib))

    ifeq ($(os), macosx)
        $$(prorab_this_staticlib): $$(this_mm_obj)
        $$(prorab_this_name): $$(this_mm_obj)
    endif

    $$(eval $$(prorab-clear-this-vars))
endef

ifeq ($(os), linux)
    $(eval $(call ruisapp_rules,opengl,x11))
    $(eval $(call ruisapp_rules,opengles,x11))
    $(eval $(call ruisapp_rules,opengl,wayland))
    $(eval $(call ruisapp_rules,opengles,wayland))
else
    $(eval $(call ruisapp_rules,opengl))
endif

# clear variable
ruisapp_rules :=

this_license_file := ../LICENSE
this_src_dir := ruisapp
$(eval $(prorab-license))

$(eval $(prorab-clang-format))
