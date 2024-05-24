include prorab.mk
include prorab-test.mk

this_name := ruisapp-test

$(eval $(call prorab-config, ../../config))

this_srcs += $(call prorab-src-dir, src)

this_cxxflags += -I ../../src

ifeq ($(os),linux)
    this__cfg_suffix := $(if $(ogles2),opengles,opengl)-$(if $(wayland),wayland,xorg)
    this_libruisapp := libruisapp-$(this__cfg_suffix)
else
    this__cfg_suffix := $(if $(ogles2),opengles,opengl)
    this_libruisapp := libruisapp-opengl
endif

this_libruisapp := ../../src/out/$(c)/$(this__cfg_suffix)/$(this_libruisapp)$(dot_so)

this_ldlibs += $(this_libruisapp)

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

this_ldlibs += -ltml -lruis -lutki -lpapki -lm

$(eval $(prorab-build-app))

this_run_name := app
this_test_cmd := $(prorab_this_name)
this_test_deps := $(prorab_this_name)
this_test_ld_path := ../../src/out/$(c)/$(this__cfg_suffix)/
$(eval $(prorab-run))

$(eval $(call prorab-include, ../../src/makefile))
