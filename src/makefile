include prorab.mk
include $(d)../prorab-config.mk

include $(d)../raspberrypi.mk

$(eval $(call prorab-config, ../config/the_lib))

this_soname := $(shell cat $(d)/soname.txt)

this_srcs += $(call prorab-src-dir, .)

this_name := mordavokne-opengl2

ifeq ($(os), macosx)
    this_mm_obj := $(d)$(this_out_dir)obj_$(this_name)/objc/mordavokne/glue/macosx/glue.mm.o

    define this_rules
        $(this_mm_obj): $(d)mordavokne/glue/macosx/glue.mm
$(.RECIPEPREFIX)@echo compile $$<...
$(.RECIPEPREFIX)$(a)mkdir -p $$(dir $$@)
$(.RECIPEPREFIX)$(a)$(this_cc) -ObjC++ -std=c++11 -g -c -o "$$@" $(this_objcflags) $$<
    endef
    $(eval $(this_rules))
endif

this_cxxflags += -DMORDAVOKNE_RENDER_OPENGL2
this_ldlibs += -lmorda-opengl2-ren

$(eval $(prorab-build-lib))

ifeq ($(os), macosx)
    $(prorab_this_staticlib): $(this_mm_obj)
    $(prorab_this_name): $(this_mm_obj)
endif

$(eval $(prorab-clear-this-vars))

ifeq ($(os), linux)
    ogles2 := true
endif

ifeq ($(ogles2), true)
    $(eval $(call prorab-config, ../config/the_lib))

    this_name := mordavokne-opengles2

    this_soname := $(shell cat $(d)/soname.txt)

    this_srcs += $(call prorab-src-dir, .)

    this_cxxflags += -DMORDAVOKNE_RENDER_OPENGLES2
    this_ldlibs += -lmorda-opengles2-ren

    ifeq ($(raspberrypi), true)
        this_cxxflags += -DM_RASPBERRYPI -I/opt/vc/include
        this_ldflags += -L/opt/vc/lib
        this_ldlibs += -lbrcmEGL
    else
        this_ldlibs += -lEGL
    endif

    $(eval $(prorab-build-lib))
endif
