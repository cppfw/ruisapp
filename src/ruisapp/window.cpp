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

#include "window.hpp"

using namespace ruisapp;

window::window(utki::shared_ref<ruis::context> ruis_context) :
	gui(std::move(ruis_context))
{}

void window::render()
{
	// TODO: bind render context

	// TODO: render only if needed?
	this->gui.context.get().renderer.get().render_context.get().clear_framebuffer_color();

	// no clear of depth and stencil buffers, it will be done by individual widgets if needed

	this->gui.render(this->gui.context.get().renderer.get().render_context.get().initial_matrix);

	this->gui.context.get().window().swap_frame_buffers();
}

void window::update_window_rect(const ruis::rect& rect)
{
	if (this->cur_window_rect == rect) {
		return;
	}

	this->cur_window_rect = rect;

	utki::log_debug([&](auto& o) {
		o << "application::update_window_rect(): this->cur_window_rect = " << this->cur_window_rect << std::endl;
	});

	this->gui.context.get().renderer.get().render_context.get().set_viewport(r4::rectangle<uint32_t>(
		uint32_t(this->cur_window_rect.p.x()),
		uint32_t(this->cur_window_rect.p.y()),
		uint32_t(this->cur_window_rect.d.x()),
		uint32_t(this->cur_window_rect.d.y())
	));

	this->gui.set_viewport(this->cur_window_rect.d);
}
