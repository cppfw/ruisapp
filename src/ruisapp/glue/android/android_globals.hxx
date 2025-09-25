#pragma once

#include <android/native_activity.h>

#include "../../application.hpp"

namespace {
struct android_globals_wrapper final {
    static ANativeActivity* native_activity = nullptr;

    // static void android_globals_wrapper::

    static android_globals_wrapper& get(){
        utki::assert(native_activity, SL);
        utki::assert(native_activity->instance, SL);
    }

	std::unique_ptr<ruisapp::application> app;
};
} // namespace
