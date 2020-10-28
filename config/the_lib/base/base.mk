include $(config_dir)../base/base.mk

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
