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

#include <set>

#include "wayland_compositor.hxx"
#include "wayland_region.hxx"

namespace {
struct wayland_surface_wrapper {
	wl_surface* const surface;

	wayland_surface_wrapper(const wayland_compositor_wrapper& wayland_compositor) :
		surface(wl_compositor_create_surface(wayland_compositor.compositor))
	{
		if (!this->surface) {
			throw std::runtime_error("could not create wayland surface");
		}

		wl_surface_add_listener(
			this->surface, //
			&listener,
			this
		);
	}

	wayland_surface_wrapper(const wayland_surface_wrapper&) = delete;
	wayland_surface_wrapper& operator=(const wayland_surface_wrapper&) = delete;

	wayland_surface_wrapper(wayland_surface_wrapper&&) = delete;
	wayland_surface_wrapper& operator=(wayland_surface_wrapper&&) = delete;

	~wayland_surface_wrapper()
	{
		wl_surface_destroy(this->surface);
	}

	void commit()
	{
		wl_surface_commit(this->surface);
	}

	void damage(const r4::vector2<int32_t>& dims);

	void set_buffer_scale(uint32_t scale);
	void set_opaque_region(const wayland_region_wrapper& wayland_region);

	struct scale_and_dpi {
		uint32_t scale = 1;

		constexpr static const float default_dpi = 96;
		float dpi = default_dpi;
	};

	scale_and_dpi find_scale_and_dpi(const std::list<wayland_output_wrapper>& wayland_outputs);

private:
	// wayland outputs this surface is on
	std::set<wl_output*> wayland_outputs;

	static void wl_surface_enter(
		void* data, //
		wl_surface* surface,
		wl_output* output
	);

	static void wl_surface_leave(
		void* data, //
		wl_surface* surface,
		wl_output* output
	);

	constexpr static const wl_surface_listener listener = {
		.enter = &wl_surface_enter, //
		.leave = &wl_surface_leave
	};
};
} // namespace
