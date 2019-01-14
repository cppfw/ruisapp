include prorab.mk

include $(d)../../raspberrypi.mk

this_name := mordavokne-test


this_srcs += src/main.cpp


this_cxxflags += -Wall
this_cxxflags += -Wno-comment #no warnings on nested comments
this_cxxflags += -Wno-format
this_cxxflags += -DDEBUG
this_cxxflags += -fstrict-aliasing #strict aliasing!!!
this_cxxflags += -g
this_cxxflags += -O3
this_cxxflags += -std=c++11

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
endif

ifeq ($(prorab_os),windows)
    this_ldlibs += -lmingw32 #these should go first, otherwise linker will complain about undefined reference to WinMain
    this_ldlibs += $(d)../../src/$(this_mordavoknelib)$(soext)
    this_ldlibs +=  -lglew32 -lopengl32 -lpng -ljpeg -lz -lfreetype -mwindows
else ifeq ($(prorab_os),macosx)
    this_ldlibs += $(prorab_this_dir)../../src/$(this_mordavoknelib)$(prorab_lib_extension) -lGLEW -framework OpenGL -framework Cocoa -lpng -ljpeg -lfreetype
    this_ldflags += -rdynamic
else ifeq ($(prorab_os),linux)
    this_ldlibs += $(prorab_this_dir)../../src/$(this_mordavoknelib)$(prorab_lib_extension) -pthread
    this_ldflags += -rdynamic
endif

this_ldlibs += -lstob -lmorda -lunikod -lm -lstdc++

$(eval $(prorab-build-app))


define this_rules
run:: $(prorab_this_name)
	@echo running $$^...
	@(cd $(d); LD_LIBRARY_PATH=../../src $$^)
endef
$(eval $(this_rules))


#add dependency on libmordavokne
ifeq ($(prorab_os),windows)
    $(prorab_this_dir)$(this_mordavoknelib)$(prorab_lib_extension): $(abspath $(prorab_this_dir)../../src/$(this_mordavoknelib)$(prorab_lib_extension))
	@cp $< $@

    $(prorab_this_name): $(prorab_this_dir)$(this_mordavoknelib)$(prorab_lib_extension)

    define this_rules
        clean::
		@rm -f $(prorab_this_dir)$(this_mordavoknelib)$(prorab_lib_extension)
    endef
    $(eval $(this_rules))
else
    $(prorab_this_name): $(abspath $(d)../../src/$(this_mordavoknelib)$(soext))
endif


$(eval $(call prorab-include,$(prorab_this_dir)../../src/makefile))
