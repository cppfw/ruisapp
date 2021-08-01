include prorab.mk
include prorab-test.mk

this_name := mordavokne-test

$(eval $(call prorab-config, ../../config))

this_srcs += $(call prorab-src-dir, src)

ifeq ($(ogles2), true)
    this_mordavoknelib := libmordavokne-opengles
else
    this_mordavoknelib := libmordavokne-opengl
endif

this_mordavoknelib := ../../src/out/$(c)/$(this_mordavoknelib)$(dot_so)

this_ldlibs += $(this_mordavoknelib)

ifeq ($(os),windows)
    this_ldlibs += -lmingw32 # these should go first, otherwise linker will complain about undefined reference to WinMain
    this_ldlibs += -lglew32 -lopengl32 -lpng -ljpeg -lz -lfreetype -mwindows
else ifeq ($(os),macosx)
    this_ldlibs += -lGLEW -framework OpenGL -framework Cocoa -lpng -ljpeg -lfreetype
    this_ldflags += -rdynamic
else ifeq ($(os),linux)
    this_ldlibs += -pthread
    this_ldflags += -rdynamic
endif

this_ldlibs += -ltreeml -lmorda -lutki -lpapki -lm

$(eval $(prorab-build-app))

$(eval $(call prorab-depend, $(prorab_this_name), $(this_mordavoknelib)))

this_run_name := app
this_test_cmd := $(prorab_this_name)
this_test_deps := $(prorab_this_name)
this_test_ld_path := ../../src/out/$(c)
$(eval $(prorab-run))

$(eval $(call prorab-include, ../../src/makefile))
