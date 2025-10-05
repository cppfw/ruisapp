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
	xdg_surface* surface;

	constexpr static const xdg_surface_listener listener = {
		.configure =
			[](void* data, //
			   xdg_surface* xdg_surface,
			   uint32_t serial) {
				xdg_surface_ack_configure(
					xdg_surface, //
					serial
				);
			}, //
	};

	xdg_surface_wrapper(
		wayland_surface_wrapper& wayland_surface, //
		xdg_wm_base_wrapper& xdg_wm_base
	) :
		surface(xdg_wm_base_get_xdg_surface(
			xdg_wm_base.wm_base, //
			wayland_surface.surface
		))
	{
		if (!this->surface) {
			throw std::runtime_error("could not create wayland xdg surface");
		}

		xdg_surface_add_listener(
			this->surface, //
			&listener,
			nullptr
		);
	}

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
