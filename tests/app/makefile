include prorab.mk

include $(d)../../raspberrypi.mk

this_name := mordavokne-test

this_out_dir := build

this_srcs += src/main.cpp

this_cxxflags += -Wall
this_cxxflags += -Wno-comment # no warnings on nested comments
this_cxxflags += -Werror
this_cxxflags += -DDEBUG
this_cxxflags += -fstrict-aliasing
this_cxxflags += -g
this_cxxflags += -std=c++14

ifeq ($(ogles2), true)
    this_mordavoknelib := libmordavokne-opengles2
#    this_ldlibs += -lGLESv2

#    ifeq ($(raspberrypi), true)
#        this_ldlibs += -L/opt/vc/lib
#    endif

else
    this_mordavoknelib := libmordavokne-opengl2
#    this_ldlibs += -lGL
endif


ifeq ($(debug), true)
    this_cxxflags += -DDEBUG
else
    this_cxxflags += -O3
endif

this_ldlibs += $(d)../../src/build/$(this_mordavoknelib)$(soext)

ifeq ($(prorab_os),windows)
    this_ldlibs += -lmingw32 #these should go first, otherwise linker will complain about undefined reference to WinMain
    this_ldlibs += -lglew32 -lopengl32 -lpng -ljpeg -lz -lfreetype -mwindows
else ifeq ($(prorab_os),macosx)
    this_ldlibs += -lGLEW -framework OpenGL -framework Cocoa -lpng -ljpeg -lfreetype
    this_ldflags += -rdynamic
else ifeq ($(prorab_os),linux)
    this_ldlibs += -pthread
    this_ldflags += -rdynamic
endif

this_ldlibs += -lpuu -lmorda -lutki -lpapki -lm -lstdc++


$(eval $(prorab-build-app))


define this_rules
run:: $(prorab_this_name)
	@echo running $$^...
	@(cd $(d); LD_LIBRARY_PATH=../../src/build $$^)
endef
$(eval $(this_rules))


# add dependency on libmordavokne
ifeq ($(prorab_os),windows)
    $(d)$(this_mordavoknelib)$(soext): $(abspath $(d)../../src/build/$(this_mordavoknelib)$(soext))
	@cp $< $@

    $(prorab_this_name): $(d)$(this_mordavoknelib)$(soext)

    define this_rules
        clean::
		@rm -f $(d)$(this_mordavoknelib)$(soext)
    endef
    $(eval $(this_rules))
else
    $(prorab_this_name): $(abspath $(d)../../src/build/$(this_mordavoknelib)$(soext))
endif


$(eval $(call prorab-try-include,$(d)../../src/makefile))
