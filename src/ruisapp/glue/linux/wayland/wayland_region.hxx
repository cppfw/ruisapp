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

#include <r4/rectangle.hpp>

#include "wayland_compositor.hxx"

namespace {
struct wayland_region_wrapper {
	wl_region* region;

	wayland_region_wrapper(const wayland_compositor_wrapper& wayland_compositor) :
		region(wl_compositor_create_region(wayland_compositor.compositor))
	{
		if (!this->region) {
			throw std::runtime_error("could not create wayland region");
		}
	}

	wayland_region_wrapper(const wayland_region_wrapper&) = delete;
	wayland_region_wrapper& operator=(const wayland_region_wrapper&) = delete;

	wayland_region_wrapper(wayland_region_wrapper&&) = delete;
	wayland_region_wrapper& operator=(wayland_region_wrapper&&) = delete;

	~wayland_region_wrapper()
	{
		wl_region_destroy(this->region);
	}

	void add(const r4::rectangle<int32_t>& rect)
	{
		wl_region_add(
			this->region, //
			rect.p.x(),
			rect.p.y(),
			rect.d.x(),
			rect.d.y()
		);
	}
};
} // namespace
