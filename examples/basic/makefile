include prorab.mk

this_name := basic


this_srcs := src/main.cpp


this_cxxflags := -Wall
this_cxxflags += -Wno-comment #no warnings on nested comments
this_cxxflags += -Wno-format
this_cxxflags += -DDEBUG
this_cxxflags += -fstrict-aliasing #strict aliasing!!!
this_cxxflags += -g
this_cxxflags += -O2
this_cxxflags += -std=c++11


ifeq ($(debug), true)
    this_cxxflags += -DDEBUG
endif

ifeq ($(prorab_os),windows)
    this_ldlibs += -lmingw32 #these should go first, otherwise linker will complain about undefined reference to WinMain
#    this_ldlibs += $(prorab_this_dir)../../src/libruis$(prorab_lib_extension)
    this_ldflags += -L/usr/lib -L/usr/local/lib
    this_ldlibs +=  -lglew32 -lopengl32 -lz -lfreetype -mwindows

    this_cxxflags += -I/usr/include -I/usr/local/include

    #WORKAROUND for MinGW bug:
    this_cxxflags += -D__STDC_FORMAT_MACROS
else ifeq ($(prorab_os),macosx)
    this_ldlibs += -lGLEW -framework OpenGL -framework Cocoa -lfreetype
    this_ldlibs += -lruisapp
else ifeq ($(prorab_os),linux)
    this_ldlibs += -lGLEW -pthread -lGL
    this_ldlibs += `pkg-config ruisapp-opengl2 --libs`
endif

this_ldlibs += -lnitki -lopros -lstob -lruis -lstdc++



$(eval $(prorab-build-app))
