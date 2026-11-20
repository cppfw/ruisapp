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

#include <ruis/render/native_window.hpp>

#include "display.hxx"
#include "wayland_egl_window.hxx"
#include "wayland_surface.hxx"
#include "xdg_surface.hxx"
#include "xdg_toplevel.hxx"

namespace {
// Base class for the Wayland native window. Contains the common Wayland
// surface, xdg-toplevel, and cursor handling. The graphics-specific code
// (EGL or Vulkan) is in the derived leaf classes.
class wayland_window : public ruis::render::native_window
{
protected:
	const utki::shared_ref<display_wrapper> display;
	wayland_surface_wrapper wayland_surface;
	xdg_surface_wrapper xdg_surface;
	xdg_toplevel_wrapper xdg_toplevel;
	wayland_egl_window_wrapper wayland_egl_window;
	wayland_surface_wrapper::scale_and_dpi scale_and_dpi;
	ruis::mouse_cursor cur_mouse_cursor = ruis::mouse_cursor::arrow;
	bool mouse_cursor_visible = true;

public:
	// keep track of current window dimensions for restoring them after fullscreen mode
	r4::vector2<uint32_t> cur_window_dims;

	// save window dimentions before makeing it fullscreen to restore the dimentsions
	// when window is made non-fullscreen again
	r4::vector2<uint32_t> pre_fullscreen_win_dims = this->cur_window_dims;

	using window_id_type = const wl_surface*;

	const unsigned sequence_number = []() {
		static unsigned next_sequence_number = 0;
		auto ret = next_sequence_number;
		++next_sequence_number;
		return ret;
	}();

	wayland_window(utki::shared_ref<display_wrapper> display, const ruisapp::window_parameters& window_params) :
		display(std::move(display)),
		wayland_surface(this->display.get().wayland_compositor),
		xdg_surface(
			this->wayland_surface, //
			this->display.get().xdg_wm_base
		),
		xdg_toplevel(
			this->wayland_surface, //
			this->xdg_surface,
			window_params
		),
		wayland_egl_window(
			this->wayland_surface, //
			window_params.dims
		),
		cur_window_dims(window_params.dims)
	{
		utki::log_debug([](auto& o) {
			o << "wayland_window constructed" << std::endl;
		});
	}

	void resize(const r4::vector2<uint32_t>& dims);

	ruis::real get_scale() const noexcept
	{
		return ruis::real(this->scale_and_dpi.scale);
	}

	ruis::real get_dpi() const noexcept
	{
		return this->scale_and_dpi.dpi;
	}

	window_id_type get_id() const noexcept
	{
		return this->wayland_surface.surface;
	}

	// get the raw wl_surface pointer for Vulkan surface creation
	wl_surface* get_wl_surface() const noexcept
	{
		return this->wayland_surface.surface;
	}

	// get the raw wl_display pointer for Vulkan surface creation
	wl_display* get_wl_display() const noexcept
	{
		return this->display.get().wayland_display.display;
	}

	r4::vector2<unsigned> get_dims() const noexcept override
	{
		return this->cur_window_dims * this->scale_and_dpi.scale;
	}

	void set_fullscreen_internal(bool enable) override
	{
		if (enable) {
			this->pre_fullscreen_win_dims = this->cur_window_dims;
			utki::log_debug([&](auto& o) {
				o << " old win dims = " << std::dec << this->pre_fullscreen_win_dims << std::endl;
			});
			xdg_toplevel_set_fullscreen(
				this->xdg_toplevel.toplevel, //
				nullptr // output
			);
		} else {
			xdg_toplevel_unset_fullscreen(this->xdg_toplevel.toplevel);
		}
	}

	void update_mouse_cursor()
	{
		auto& wayland_pointer = this->display.get().wayland_seat.wayland_pointer;

		if (this->mouse_cursor_visible) {
			wayland_pointer.set_cursor(this->cur_mouse_cursor);
		} else {
			wayland_pointer.set_cursor(ruis::mouse_cursor::none);
		}
	}

	void set_mouse_cursor_visible(bool visible) override
	{
		this->mouse_cursor_visible = visible;
		this->update_mouse_cursor();
	}

	void set_mouse_cursor(ruis::mouse_cursor cursor) override
	{
		this->cur_mouse_cursor = cursor;
		this->update_mouse_cursor();
	}

	wl_callback* make_frame_callback()
	{
		return wl_surface_frame(this->wayland_surface.surface);
	}

	void mark_dirty()
	{
		this->wayland_surface.damage(this->cur_window_dims.to<int32_t>());
	}
};
} // namespace
