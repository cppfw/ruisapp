include prorab.mk
include $(d)../prorab-config.mk

include $(d)../raspberrypi.mk

define this_rules

$(call prorab-config, ../config)

this_soname := $(shell cat $(d)soname.txt)

this_srcs += $(call prorab-src-dir, .)

this_name := mordavokne-$1

ifeq ($(os), linux)
    this_cxxflags += -fPIC # generate position independent code
    this_ldlibs += -lGLEW -ldl -lnitki -lopros -lX11
else ifeq ($(os), windows)
    this_ldlibs += -lgdi32 -lopengl32 -lglew32
else ifeq ($(os), macosx)
    this_cxxflags += -stdlib=libc++ # this is needed to be able to use c++11 std lib
    this_ldlibs += -lGLEW -lc++
    this_ldlibs += -framework Cocoa -framework OpenGL -ldl
endif

this_ldlibs += -lmorda -lpapki -lpuu -lstdc++

ifeq ($(os), macosx)
    this_mm_obj := $(d)$(this_out_dir)obj_$(this_name)/objc/mordavokne/glue/macosx/glue.mm.o

        $(this_mm_obj): $(d)mordavokne/glue/macosx/glue.mm
$(.RECIPEPREFIX)@echo compile $$<...
$(.RECIPEPREFIX)$(a)mkdir -p $$(dir $$@)
$(.RECIPEPREFIX)$(a)$(this_cc) -ObjC++ -std=c++11 -g -c -o "$$@" $(this_objcflags) $$<

endif

this_cxxflags += -DMORDAVOKNE_RENDER_$(shell echo $1 | tr '[:lower:]' '[:upper:]')
this_ldlibs += -lmorda-$1-ren

ifeq ($1,opengles2)
    ifeq ($(raspberrypi), true)
        this_cxxflags += -DM_RASPBERRYPI -I/opt/vc/include
        this_ldflags += -L/opt/vc/lib
        this_ldlibs += -lbrcmEGL
    else
        this_ldlibs += -lEGL
    endif
endif

$(prorab-build-lib)

ifeq ($(os), macosx)
    $(prorab_this_staticlib): $(this_mm_obj)
    $(prorab_this_name): $(this_mm_obj)
endif

$(prorab-clear-this-vars)

endef

$(info $(call this_rules,opengl2))

$(eval $(call this_rules,opengl2))

ifeq ($(os), linux)
    $(eval $(call this_rules,opengles2))
endif


# ifeq ($(ogles2), true)
#     $(eval $(call prorab-config, ../config))

#     this_name := mordavokne-opengles2

#     this_soname := $(shell cat $(d)/soname.txt)

#     this_srcs += $(call prorab-src-dir, .)

# ifeq ($(os), linux)
#     this_cxxflags += -fPIC # generate position independent code
#     this_ldlibs += -lGLEW -ldl -lnitki -lopros -lX11
# else ifeq ($(os), windows)
#     this_ldlibs += -lgdi32 -lopengl32 -lglew32
# else ifeq ($(os), macosx)
#     this_cxxflags += -stdlib=libc++ # this is needed to be able to use c++11 std lib
#     this_ldlibs += -lGLEW -lc++
#     this_ldlibs += -framework Cocoa -framework OpenGL -ldl
# endif

# this_ldlibs += -lmorda -lpapki -lpuu -lstdc++


#     this_cxxflags += -DMORDAVOKNE_RENDER_OPENGLES2
#     this_ldlibs += -lmorda-opengles2-ren

#     ifeq ($(raspberrypi), true)
#         this_cxxflags += -DM_RASPBERRYPI -I/opt/vc/include
#         this_ldflags += -L/opt/vc/lib
#         this_ldlibs += -lbrcmEGL
#     else
#         this_ldlibs += -lEGL
#     endif

#     $(eval $(prorab-build-lib))
# endif
