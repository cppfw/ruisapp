#pragma once

#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/native_activity.h>
#include <android/window.h>
#include <nitki/queue.hpp>
#include <utki/debug.hpp>

#include "../../application.hpp"

#include "java_functions.hxx"

namespace {
struct android_globals_wrapper final {
	static ANativeActivity* native_activity = nullptr;

	static void create(ANativeActivity* native_activity);
	static void destroy();

	static android_globals_wrapper& get()
	{
		utki::assert(native_activity, SL);
		utki::assert(native_activity->instance, SL);
		return
	}

	java_functions_wrapper java_functions;

	nitki::queue ui_queue;

	std::unique_ptr<ruisapp::application> app;

	ANativeWindow* android_window = nullptr;
};
} // namespace

namespace {
inline android_globals_wrapper& get_glob()
{
	auto na = android_globals_wrapper::native_activity;
	utki::assert(na, SL);
	utki::assert(na->instance, SL);
	return *static_cast<android_globals_wrapper*>(na->instance);
}
} // namespace
