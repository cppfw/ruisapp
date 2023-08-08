include prorab.mk
include prorab-license.mk
include prorab-linux.mk
include prorab-clang-format.mk

define mordavokne_rules

    $$(eval $$(call prorab-config, ../config))

    this_soname := $(shell cat $(d)soname.txt)

    this_srcs += $$(call prorab-src-dir, .)

    this_name := mordavokne-$1

    ifeq ($(os), linux)
        this_ldlibs += -lGLEW -ldl -lnitki -lopros -lX11
    else ifeq ($(os), windows)
        this_ldlibs += -lgdi32 -lopengl32 -lglew32
    else ifeq ($(os), macosx)
        this_ldlibs += -lGLEW
        this_ldlibs += -framework Cocoa -framework OpenGL -ldl
    endif

    this_ldlibs += -lmorda -lpapki -ltreeml

    ifeq ($(os), macosx)
        this_mm_obj := $$(d)$$(this_out_dir)obj_$$(this_name)/objc/mordavokne/glue/macosx/glue.mm.o

        define this_subrules
            $$(this_mm_obj): $$(d)mordavokne/glue/macosx/glue.mm
$(.RECIPEPREFIX)@echo compile $$$$<...
$(.RECIPEPREFIX)$(a)mkdir -p $$$$(dir $$$$@)
$(.RECIPEPREFIX)$(a)$(this_cc) -ObjC++ -c -o "$$$$@" $$(this_cxxflags) $$(OBJCXXFLAGS) $$ $$$$<
        endef
        $$(eval $$(this_subrules))

    endif

    this_cxxflags += -DMORDAVOKNE_RENDER_$(shell echo $1 | tr '[:lower:]' '[:upper:]')
    this_ldlibs += -lmorda-render-$1

    ifeq ($1,opengles)
        ifeq ($(prorab_linux),raspbian)
            this_cxxflags += -DMORDAVOKNE_RASPBERRYPI -isystem /opt/vc/include
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

$(eval $(call mordavokne_rules,opengl))

ifeq ($(os), linux)
    $(eval $(call mordavokne_rules,opengles))
endif

# clear variable
mordavokne_rules :=

this_license_file := ../LICENSE
this_src_dir := .
$(eval $(prorab-license))

$(eval $(prorab-clang-format))
