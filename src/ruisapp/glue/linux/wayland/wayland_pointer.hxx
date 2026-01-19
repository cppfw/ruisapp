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

#include <ruis/config.hpp>
#include <ruis/util/events.hpp>

#include "wayland_cursor_theme.hxx"
#include "wayland_surface.hxx"

namespace {
struct wayland_pointer_wrapper {
	wayland_cursor_theme_wrapper wayland_cursor_theme;

	wayland_surface_wrapper cursor_wayland_surface;

	// Current pointer position within current surface.
	ruis::vec2 cur_pointer_pos{0, 0};

	void connect(wl_seat* seat);
	void disconnect() noexcept;

	void set_cursor(ruis::mouse_cursor c);

	wayland_pointer_wrapper(
		const wayland_compositor_wrapper& wayland_compositor, //
		const wayland_shm_wrapper& wayland_shm
	) :
		wayland_cursor_theme(wayland_shm),
		cursor_wayland_surface(wayland_compositor)
	{}

	wayland_pointer_wrapper(const wayland_pointer_wrapper&) = delete;
	wayland_pointer_wrapper& operator=(const wayland_pointer_wrapper&) = delete;

	wayland_pointer_wrapper(wayland_pointer_wrapper&&) = delete;
	wayland_pointer_wrapper& operator=(wayland_pointer_wrapper&&) = delete;

	~wayland_pointer_wrapper()
	{
		if (this->pointer) {
			this->release();
		}
	}

private:
	void release()
	{
		if (wl_pointer_get_version(this->pointer) >= WL_POINTER_RELEASE_SINCE_VERSION) {
			wl_pointer_release(this->pointer);
		} else {
			wl_pointer_destroy(this->pointer);
		}
	}

	static void wl_pointer_enter(
		void* data, //
		wl_pointer* pointer,
		uint32_t serial,
		wl_surface* surface,
		wl_fixed_t x,
		wl_fixed_t y
	);

	static void wl_pointer_leave(
		void* data, //
		wl_pointer* pointer,
		uint32_t serial,
		wl_surface* surface
	);

	static void wl_pointer_motion(
		void* data, //
		wl_pointer* pointer,
		uint32_t time,
		wl_fixed_t x,
		wl_fixed_t y
	);

	static void wl_pointer_button(
		void* data, //
		wl_pointer* pointer,
		uint32_t serial,
		uint32_t time,
		uint32_t button,
		uint32_t state
	);

	static void wl_pointer_axis(
		void* data, //
		wl_pointer* pointer,
		uint32_t time,
		uint32_t axis,
		wl_fixed_t value
	);

	constexpr static const wl_pointer_listener listener = {
		.enter = &wl_pointer_enter,
		.leave = &wl_pointer_leave,
		.motion = &wl_pointer_motion,
		.button = &wl_pointer_button,
		.axis = &wl_pointer_axis,
		.frame =
			[](void* data, //
			   wl_pointer* pointer) {
				utki::log_debug([](auto& o) {
					o << "pointer frame" << std::endl;
				});
			},
		.axis_source =
			[](void* data, //
			   wl_pointer* pointer,
			   uint32_t source) {
				utki::log_debug([&](auto& o) {
					o << "axis source: " << std::dec << source << std::endl;
				});
			},
		.axis_stop =
			[](void* data, //
			   wl_pointer* pointer,
			   uint32_t time,
			   uint32_t axis) {
				utki::log_debug([&](auto& o) {
					o << "axis stop: axis = " << std::dec << axis << std::endl;
				});
			},
		.axis_discrete =
			[](void* data, //
			   wl_pointer* pointer,
			   uint32_t axis,
			   int32_t discrete) {
				utki::log_debug([&](auto& o) {
					o << "axis discrete: axis = " << std::dec << axis << ", discrete = " << discrete << std::endl;
				});
			}
	};

	unsigned num_connected = 0;

	wl_pointer* pointer = nullptr;

	wl_surface* cur_surface = nullptr;

	uint32_t last_enter_event_serial_number = 0;

	void apply_cursor(wl_cursor* cursor);
};
} // namespace
