include prorab.mk

include $(d)../raspberrypi.mk

define mordavokne_rules

    $$(eval $$(call prorab-config, ../config))

    this_soname := $(shell cat $(d)soname.txt)

    this_srcs += $$(call prorab-src-dir, .)

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

    this_ldlibs += -lmorda -lpapki -ltreeml -lstdc++

    ifeq ($(os), macosx)
        this_mm_obj := $$(d)$$(this_out_dir)obj_$$(this_name)/objc/mordavokne/glue/macosx/glue.mm.o

        define this_subrules
            $$(this_mm_obj): $$(d)mordavokne/glue/macosx/glue.mm
$(.RECIPEPREFIX)@echo compile $$$$<...
$(.RECIPEPREFIX)$(a)mkdir -p $$$$(dir $$$$@)
$(.RECIPEPREFIX)$(a)$(this_cc) -ObjC++ -std=c++11 -g -c -o "$$$$@" $$(this_objcflags) $$$$<
        endef
        $$(eval $$(this_subrules))

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

    $$(eval $$(prorab-build-lib))

    ifeq ($(os), macosx)
        $$(prorab_this_staticlib): $$(this_mm_obj)
        $$(prorab_this_name): $$(this_mm_obj)
    endif

    $$(eval $$(prorab-clear-this-vars))
endef

$(eval $(call mordavokne_rules,opengl2))

ifeq ($(os), linux)
    $(eval $(call mordavokne_rules,opengles2))
endif

# clear variable
mordavokne_rules :=
