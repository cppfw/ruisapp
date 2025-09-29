#pragma once

#include <android/native_activity.h>
#include <android/window.h>
#include <nitki/queue.hpp>
#include <utki/debug.hpp>
#include <utki/unique_ref.hpp>

#include "../../application.hpp"

#include "android_configuration.hxx"
#include "event_fd.hxx"
#include "java_functions.hxx"
#include "linux_timer.hxx"

namespace {
struct globals_wrapper final {
	static ANativeActivity* native_activity = nullptr;

	static void create(ANativeActivity* activity);
	static void destroy();

	static globals_wrapper& get()
	{
		utki::assert(native_activity, SL);
		utki::assert(native_activity->instance, SL);
		return
	}

	globals_wrapper();

	globals_wrapper(const globals_wrapper&) = delete;
	globals_wrapper& operator=(const globals_wrapper&) = delete;

	globals_wrapper(globals_wrapper&&) = delete;
	globals_wrapper& operator=(globals_wrapper&&) = delete;

	~globals_wrapper();

	java_functions_wrapper java_functions;

	// save pointer to current thread's looper
	ALooper* looper = []() {
		auto l = ALooper_prepare(0);
		utki::assert(l, SL);
		return l;
	}();

	event_fd_wrapper fd_flag; // TODO: rename to main_loop_event_fd
	linux_timer timer{[&]() {
		this->fd_flag.set();
	}};

	nitki::queue ui_queue;

	utki::unique_ref<android_configuration_wrapper> cur_android_configuration =
		utki::make_unique<android_configuration_wrapper>(*android_configuration_wrapper::native_activity->assetManager);

	AInputQueue* input_queue = nullptr;

	ruis::vector2 cur_window_dims(0, 0);

	utki::unique_ref<ruisapp::application> app;
};
} // namespace

namespace {
inline globals_wrapper& get_glob()
{
	auto na = globals_wrapper::native_activity;
	utki::assert(na, SL);
	utki::assert(na->instance, SL);
	return *static_cast<globals_wrapper*>(na->instance);
}
} // namespace
