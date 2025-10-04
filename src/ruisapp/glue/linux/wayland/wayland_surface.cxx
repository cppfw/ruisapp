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

#include "wayland_surface.hxx"

#include "application.hxx"

void wayland_surface_wrapper::wl_surface_enter(
	void* data, //
	wl_surface* surface,
	wl_output* output
)
{
	utki::log_debug([&](auto& o) {
		o << "surface enters output(" << wayland_output_wrapper::get_output_id(output) << ")" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_surface_wrapper*>(data);
	utki::assert(surface == self.surface, SL);

	utki::assert(self.wayland_outputs.find(output) == self.wayland_outputs.end(), SL);

	self.wayland_outputs.insert(output);

	auto& glue = get_glue();

	auto window = glue.get_window(self.surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_surface_wrapper::wl_surface_enter(): no window for given surface, ignoring enter output event"
			  << std::endl;
		});
		// TODO: this callback is called for some surface, try to figure out what is that surface
		// utki::assert(surface == glue.get_shared_gl_context_window_id(), SL);
		return;
	}

	window->notify_outputs_changed();
}

void wayland_surface_wrapper::wl_surface_leave(
	void* data, //
	wl_surface* surface,
	wl_output* output
)
{
	utki::log_debug([&](auto& o) {
		o << "surface leaves output(" << wayland_output_wrapper::get_output_id(output) << ")" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_surface_wrapper*>(data);
	utki::assert(surface == self.surface, SL);

	utki::assert(self.wayland_outputs.find(output) != self.wayland_outputs.end(), SL);

	self.wayland_outputs.erase(output);

	auto& glue = get_glue();

	auto window = glue.get_window(self.surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_surface_wrapper::wl_surface_leave(): no window for given surface, ignoring leave output event"
			  << std::endl;
		});
		return;
	}

	window->notify_outputs_changed();
}

wayland_surface_wrapper::scale_and_dpi //
wayland_surface_wrapper::find_scale_and_dpi( //
	const std::list<wayland_output_wrapper>& wayland_outputs
)
{
	utki::log_debug([](auto& o) {
		o << "looking for max scale" << std::endl;
	});

	// if surface did not enter any outputs yet, then just take scale of first available output
	if (this->wayland_outputs.empty()) {
		utki::log_debug([](auto& o) {
			o << "  surface has not entered any outputs yet" << std::endl;
		});
		return {};
	}

	scale_and_dpi max_sd;

	// go through outputs which the surface has entered
	for (auto wlo : this->wayland_outputs) {
		auto id = wayland_output_wrapper::get_output_id(wlo);

		utki::log_debug([&](auto& o) {
			o << "  check output id = " << id << std::endl;
		});

		auto i = std::find_if(wayland_outputs.begin(), wayland_outputs.end(), [&id](const auto& o) {
			return o.id == id;
		});
		if (i == wayland_outputs.end()) {
			utki::log_debug([&](auto& o) {
				o << "WARNING: wayland surface entered output with id = " << id
				  << ", but no output with such id is reported" << std::endl;
			});
			continue;
		}

		auto& output = *i;

		utki::log_debug([&](auto& o) {
			o << "  output found, scale = " << output.scale << std::endl;
		});

		using std::max;
		max_sd.scale = max(output.scale, max_sd.scale);
		max_sd.dpi = output.get_dpi();
	}

	return max_sd;
}

void wayland_surface_wrapper::set_buffer_scale(uint32_t scale)
{
	if (wl_surface_get_version(this->surface) >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION) {
		wl_surface_set_buffer_scale(
			this->surface, //
			int32_t(scale)
		);
	}
}

void wayland_surface_wrapper::set_opaque_region(const wayland_region_wrapper& wayland_region)
{
	wl_surface_set_opaque_region(
		this->surface, //
		wayland_region.region
	);
}

void wayland_surface_wrapper::damage(const r4::vector2<int32_t>& dims)
{
	// Use wl_surface_damage_buffer() over wl_surface_damage() as it is the
	// the modern preferred way fo marking a surface dirty.
	wl_surface_damage_buffer(
		this->surface, //
		0,
		0,
		dims.x(),
		dims.y()
	);
	this->commit();
}
