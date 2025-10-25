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

#include <xdg-shell-client-protocol.h>

#include "wayland_surface.hxx"
#include "xdg_wm_base.hxx"

namespace {
struct xdg_surface_wrapper {
	wayland_surface_wrapper& wayland_surface;

	xdg_surface* const surface;

	static void xdg_surface_configure(
		void* data, //
		xdg_surface* surface,
		uint32_t serial
	);

	constexpr static const xdg_surface_listener listener = {
		.configure = &xdg_surface_configure //
	};

	xdg_surface_wrapper(
		wayland_surface_wrapper& wayland_surface, //
		xdg_wm_base_wrapper& xdg_wm_base
	);

	xdg_surface_wrapper(const xdg_surface_wrapper&) = delete;
	xdg_surface_wrapper& operator=(const xdg_surface_wrapper&) = delete;

	xdg_surface_wrapper(xdg_surface_wrapper&&) = delete;
	xdg_surface_wrapper& operator=(xdg_surface_wrapper&&) = delete;

	~xdg_surface_wrapper()
	{
		xdg_surface_destroy(this->surface);
	}
};
} // namespace
