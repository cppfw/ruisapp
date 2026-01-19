/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2025  Ivan Gagis <igagis@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* ================ LICENSE END ================ */

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
	static ANativeActivity* native_activity;

	static void create(ANativeActivity* activity);
	static void destroy();

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

	event_fd_wrapper main_loop_event_fd;
	linux_timer timer{[&]() {
		this->main_loop_event_fd.set();
	}};

	nitki::queue ui_queue;

	utki::unique_ref<android_configuration_wrapper> cur_android_configuration =
		utki::make_unique<android_configuration_wrapper>(*globals_wrapper::native_activity->assetManager);

	AInputQueue* input_queue = nullptr;

	ruis::vec2 cur_window_dims{0, 0};

	// Array of current pointer positions, needed to detect which pointers have
	// actually moved.
	std::array<ruis::vec2, 10> pointers;

	// Application object constructor needs accessing the stuff from globals_wrapper,
	// so we need to postpone application object construction to be done after the
	// globals_wrapper object is set to the static native_activity pointer.
	// This is why we don't use unique_ref here.
	std::unique_ptr<ruisapp::application> app;
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
