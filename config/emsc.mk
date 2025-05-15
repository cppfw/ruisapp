include $(config_dir)base/base.mk

this_cxxflags += -O3

this_cxx := em++
this_cc := emcc
this_ar := emar

this_static_lib_only := true

# TODO: remove the warning suppression when the PR is merged
# Suppress version-check warning due to https://github.com/conan-io/conan-center-index/pull/26247
this_cxxflags += -Wno-version-check

this_cxxflags += -fwasm-exceptions
this_cxxflags += -sSUPPORT_LONGJMP=wasm
this_ldflags += -fwasm-exceptions
this_ldflags += -sSUPPORT_LONGJMP=wasm

this_cxxflags += -pthread
this_ldflags += -pthread

this_cxxflags += --use-port=sdl2
this_ldflags += --use-port=sdl2

sdl := true
ogles := true
