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

#include "wayland_output.hxx"

void wayland_output_wrapper::wl_output_geometry(
	void* data,
	struct wl_output* wl_output,
	int32_t x,
	int32_t y,
	int32_t physical_width,
	int32_t physical_height,
	int32_t subpixel,
	const char* make,
	const char* model,
	int32_t transform
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_output_wrapper*>(data);

	self.position = {uint32_t(x), uint32_t(y)};
	self.physical_size_mm = {uint32_t(physical_width), uint32_t(physical_height)};

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ")" << '\n' //
		  << "  physical_size_mm = " << self.physical_size_mm << '\n' //
		  << "  make = " << make << '\n' //
		  << "  model = " << model << std::endl;
	});

	// TODO: is it needed to notify about outputs changed? Aren't wayland surfaces supposed to receive enter output events?
	// auto& ww = get_impl(ruisapp::application::inst());
	// ww.notify_outputs_changed();
}

void wayland_output_wrapper::wl_output_mode(
	void* data, //
	struct wl_output* wl_output,
	uint32_t flags,
	int32_t width,
	int32_t height,
	int32_t refresh
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_output_wrapper*>(data);

	self.resolution = {uint32_t(width), uint32_t(height)};

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ") resolution = " << self.resolution << std::endl;
	});

	// TODO: is it needed to notify about outputs changed? Aren't wayland surfaces supposed to receive enter output events?
	// auto& ww = get_impl(ruisapp::application::inst());
	// ww.notify_outputs_changed();
}

void wayland_output_wrapper::wl_output_scale(
	void* data, //
	struct wl_output* wl_output,
	int32_t factor
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_output_wrapper*>(data);

	self.scale = uint32_t(std::max(factor, 1));

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ") scale = " << self.scale << std::endl;
	});

	// TODO: is it needed to notify about outputs changed? Aren't wayland surfaces supposed to receive enter output events?
	// auto& ww = get_impl(ruisapp::application::inst());
	// ww.notify_outputs_changed();
}
