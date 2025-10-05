include $(config_dir)base/base.mk

this_cxxflags += -O3

this_cxx := em++
this_cc := emcc
this_ar := emar

this_static_lib_only := true

this_cxxflags += -fwasm-exceptions
this_cxxflags += -sSUPPORT_LONGJMP=wasm
this_ldflags += -fwasm-exceptions
this_ldflags += -sSUPPORT_LONGJMP=wasm
this_ldflags += -sALLOW_MEMORY_GROWTH
this_ldflags += -sMEMORY_GROWTH_GEOMETRIC_STEP=1.0

this_cxxflags += -pthread
this_ldflags += -pthread
this_ldflags += -Wno-pthreads-mem-growth

this_cxxflags += --use-port=sdl2
this_ldflags += --use-port=sdl2

sdl := true
ogles := true
