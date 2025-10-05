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

#include "window.hxx"

void native_window::resize(const r4::vector2<uint32_t>& dims)
{
	this->cur_window_dims = dims;

	this->scale_and_dpi = this->wayland_surface.find_scale_and_dpi(this->display.get().wayland_registry.outputs);

	auto d = dims * this->scale_and_dpi.scale;

	wl_egl_window_resize(
		this->wayland_egl_window.window, //
		int(d.x()),
		int(d.y()),
		0,
		0
	);

	wayland_region_wrapper region(this->display.get().wayland_compositor);
	region.add(r4::rectangle(
		{0, 0}, //
		d.to<int32_t>()
	));

	this->wayland_surface.set_opaque_region(region);

	this->wayland_surface.set_buffer_scale(this->scale_and_dpi.scale);

	this->wayland_surface.commit();

	utki::log_debug([&](auto& o) {
		o << "final window scale = " << this->scale_and_dpi.scale << '\n';
		o << "final window dpi = " << this->scale_and_dpi.dpi << std::endl;
	});
}