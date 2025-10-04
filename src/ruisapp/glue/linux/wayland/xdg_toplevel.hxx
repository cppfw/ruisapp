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

#include <utki/debug.hpp>
#include <utki/span.hpp>
#include <xdg-shell-client-protocol.h>

#include "wayland_surface.hxx"
#include "xdg_surface.hxx"

namespace {
struct xdg_toplevel_wrapper {
	wayland_surface_wrapper& wayland_surface;

	xdg_toplevel* toplevel;

	static void xdg_toplevel_configure(
		void* data, //
		xdg_toplevel* xdg_toplevel,
		int32_t width,
		int32_t height,
		wl_array* states
	);

	static void xdg_toplevel_close(
		void* data, //
		xdg_toplevel* xdg_toplevel
	);

	constexpr static const xdg_toplevel_listener listener = {
		.configure = xdg_toplevel_configure,
		.close = &xdg_toplevel_close,
	};

	xdg_toplevel_wrapper(
		wayland_surface_wrapper& wayland_surface, //
		xdg_surface_wrapper& xdg_surface,
		const ruisapp::window_parameters& window_params
	);

	xdg_toplevel_wrapper(const xdg_toplevel_wrapper&) = delete;
	xdg_toplevel_wrapper& operator=(const xdg_toplevel_wrapper&) = delete;

	xdg_toplevel_wrapper(xdg_toplevel_wrapper&&) = delete;
	xdg_toplevel_wrapper& operator=(xdg_toplevel_wrapper&&) = delete;

	~xdg_toplevel_wrapper()
	{
		xdg_toplevel_destroy(this->toplevel);
	}
};
} // namespace
