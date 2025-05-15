include prorab.mk
include prorab-test.mk

$(eval $(call prorab-config, ../../config))

this_name := ruisapp-test

ifeq ($(config), emsc)
    this_name := $(this_name).html
endif

this_srcs += $(call prorab-src-dir, src)

this_cxxflags += -I ../../src

ifeq ($(os),linux)
    ifeq ($(wayland),true)
        this__backend := wayland
    else ifeq ($(sdl),true)
        this__backend := sdl
    else
        this__backend := xorg
    endif

    this__cfg_suffix := $(if $(ogles),opengles,opengl)-$(this__backend)
    this__libruisapp := libruisapp-$(this__cfg_suffix)
else
    this__cfg_suffix := $(if $(ogles),opengles,opengl)
    this__libruisapp := libruisapp-opengl
endif

this__libruisapp := ../../src/out/$(c)/$(this__cfg_suffix)/$(this__libruisapp)

ifeq ($(config), emsc)
    this__libruisapp := $(this__libruisapp).a
else
    this__libruisapp := $(this__libruisapp)$(dot_so)
endif

this_ldlibs += $(this__libruisapp)

ifeq ($(os),windows)
    this_ldlibs += -lmingw32 # these should go first, otherwise linker will complain about undefined reference to WinMain
    this_ldlibs += -lglew32 -lopengl32 -lz -lfreetype -mwindows
else ifeq ($(os),macosx)
    this_ldlibs += -lGLEW -framework OpenGL -framework Cocoa -lfreetype
    this_ldflags += -rdynamic
else ifeq ($(os),linux)
    this_ldlibs += -pthread
    this_ldflags += -rdynamic
endif

this_ldlibs += -l tml
this_ldlibs += -l ruis
this_ldlibs += -l utki
this_ldlibs += -l papki
this_ldlibs += -l m

ifeq ($(config), emsc)
    this_ldflags += -sWASM=1
    this_ldflags += -sSINGLE_FILE
endif

$(eval $(prorab-build-app))

this_run_name := app
this_test_cmd := $(prorab_this_name)
this_test_deps := $(prorab_this_name)
this_test_ld_path := ../../src/out/$(c)/$(this__cfg_suffix)/
$(eval $(prorab-run))

$(eval $(call prorab-include, ../../src/makefile))

# targets for building with conan and emscripten

define this__rules
conan_install:
$(.RECIPEPREFIX)@echo install conan dependencies
$(.RECIPEPREFIX)$(a)conan install $(d)../../build/emscripten --update --profile:host $(d)../../build/emscripten/conan.profile --build=missing --output-folder=$(d)out/emsc
endef
$(eval $(this__rules))
