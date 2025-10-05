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

#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#include "wayland_surface.hxx"

namespace {
struct wayland_egl_window_wrapper {
	wl_egl_window* const window;

	wayland_egl_window_wrapper(
		wayland_surface_wrapper& wayland_surface, //
		r4::vector2<unsigned> dims
	) :
		window([&]() {
			auto int_dims = dims.to<int>();

			// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
			auto w = wl_egl_window_create(
				wayland_surface.surface, //
				int_dims.x(),
				int_dims.y()
			);

			if (!w) {
				throw std::runtime_error("could not create wayland egl window");
			}
			return w;
		}())
	{}

	wayland_egl_window_wrapper(const wayland_egl_window_wrapper&) = delete;
	wayland_egl_window_wrapper& operator=(const wayland_egl_window_wrapper&) = delete;

	wayland_egl_window_wrapper(wayland_egl_window_wrapper&&) = delete;
	wayland_egl_window_wrapper& operator=(wayland_egl_window_wrapper&&) = delete;

	~wayland_egl_window_wrapper()
	{
		wl_egl_window_destroy(this->window);
	}
};
} // namespace
