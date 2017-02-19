include prorab.mk

include $(d)../raspberrypi.mk

this_soname := $(shell cat $(prorab_this_dir)/soname.txt)

this_srcs += mordavokne/App.cpp


ifeq ($(prorab_os), macosx)
    this_mm_src := mordavokne/glue/macosx/glue.mm
else
    this_srcs += mordavokne/glue/glue.cpp
endif



this_common_cxxflags += -Wall #enable all warnings
this_common_cxxflags += -Wno-comment #no warnings on nested comments
this_common_cxxflags += -funsigned-char #the 'char' type is unsigned
this_common_cxxflags += -Wnon-virtual-dtor #warn if base class has non-virtual destructor
this_common_cxxflags += -fno-operator-names #do not treat 'and', 'bitand','bitor','compl','not','or','xor' as keywords
this_common_cxxflags += -Werror #treat warnings as errors
this_common_cxxflags += -Wfatal-errors # stop on first error encountered
this_common_cxxflags += -fstrict-aliasing #strict aliasing (in order to comply with the c++ standard more strictly)
this_common_cxxflags += -g #include debugging symbols
this_common_cxxflags += -std=c++11




ifeq ($(debug),true)
    this_common_cxxflags += -DDEBUG
#    this_common_cxxflags += -DM_MORDA_RENDER_WIDGET_BORDERS
    this_objcflags += -DDEBUG
    this_common_cxxflags += -O0
else
    this_common_cxxflags += -O2 #TODO: switch to -O3 when g++ bug is fixed and it won't produce segfault
endif

this_name := mordavokne-opengl2

ifeq ($(prorab_os), linux)
    this_common_cxxflags += -fPIC #generate position independent code
    this_common_ldlibs += -ldl -lnitki -lpogodi -lX11
else ifeq ($(prorab_os), windows)
#    this_common_cxxflags += -Wno-format #due to bugs in some g++ versions it warns about format when not needed, so disable format warnings
#    this_common_cxxflags += -I/usr/include -I/usr/local/include/ -I/usr/local/include/freetype2
    this_common_cxxflags += -DM_MORDA_ENFORCE_OPENGL_UNDER_WINDOWS
#    this_ldflags += -L/usr/local/lib -L/usr/lib

    #NOTE: order matters in MinGW, libs referenced by other libs should go after those libs which reference them.
    #this_common_ldlibs += -lfreetype -ljpeg -lpng -lz -lopengl32 -lglew32 -lgdi32
else ifeq ($(prorab_os), macosx)
    this_common_cxxflags += -stdlib=libc++ #this is needed to be able to use c++11 std lib
    this_common_ldlibs += -lc++
    this_common_ldlibs += -framework Cocoa -framework OpenGL -ldl
    
    this_mm_obj := $(d)obj_$(this_name)/objc/mordavokne/glue/macosx/glue.o

    define this_rules
        $(this_mm_obj): $(d)mordavokne/glue/macosx/glue.mm
		@echo Compiling $$<...
		$(prorab_echo)mkdir -p $$(dir $$@)
		$(prorab_echo)$(CC) -ObjC++ -std=c++11 -g -c -o "$$@" $(this_objcflags) $$<
    endef
    $(eval $(this_rules))
endif

this_common_ldlibs += -lmorda -lpapki -lstob -lstdc++



ifeq ($(raspberrypi), true)
else

this_cxxflags := $(this_common_cxxflags) -DM_RENDER_OPENGL2
this_ldlibs := $(this_common_ldlibs) -lmorda-opengl2-ren -lGLEW

$(eval $(prorab-build-lib))


ifeq ($(prorab_os), macosx)
    $(prorab_this_staticlib): $(this_mm_obj)

    $(prorab_this_name): $(this_mm_obj)
endif

endif


ifeq ($(os), linux)
    ogles2 := true
endif

ifeq ($(ogles2), true)
    this_name := mordavokne-opengles2

    this_cxxflags := $(this_common_cxxflags) -DM_RENDER_OPENGLES2
    this_ldlibs := $(this_common_ldlibs) -lmorda-opengles2-ren -lEGL

    ifeq ($(raspberrypi), true)
        this_cxxflags += -DM_RASPEBRRYPI
    endif

    $(eval $(prorab-build-lib))
endif
