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

#ifdef RUISAPP_RENDER_VULKAN
#	include <vulkan/vulkan.h>
#	include <vulkan/vulkan_wayland.h>
#else
#	include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#	include "../../egl_utils.hxx"
#endif

#include "display.hxx"
#include "wayland_egl_window.hxx"
#include "wayland_surface.hxx"
#include "xdg_surface.hxx"
#include "xdg_toplevel.hxx"

namespace {
class native_window : public ruis::render::native_window
{
	const utki::shared_ref<display_wrapper> display;

	wayland_surface_wrapper wayland_surface;
	xdg_surface_wrapper xdg_surface;
	xdg_toplevel_wrapper xdg_toplevel;
	wayland_egl_window_wrapper wayland_egl_window;

#if defined(RUISAPP_RENDER_OPENGL) || defined(RUISAPP_RENDER_OPENGLES)
	egl_config_wrapper egl_config;
	egl_context_wrapper egl_context;
	// An offscreen pbuffer surface used as a fallback for the shared GL context
	// when no real window surface is available (e.g. during resource loading).
	std::optional<egl_pbuffer_surface_wrapper> egl_dummy_surface;
	std::optional<egl_surface_wrapper> egl_surface;
#endif

	wayland_surface_wrapper::scale_and_dpi scale_and_dpi;

public:
	const unsigned sequence_number = []() {
		static unsigned next_sequence_number = 0;
		auto ret = next_sequence_number;
		++next_sequence_number;
		return ret;
	}();

	// keep track of current window dimensions for restoring them after fullscreen mode
	r4::vector2<uint32_t> cur_window_dims;

	// save window dimentions before makeing it fullscreen to restore the dimentsions
	// when window is made non-fullscreen again
	r4::vector2<uint32_t> pre_fullscreen_win_dims = this->cur_window_dims;

	using window_id_type = const wl_surface*;

	native_window(
		utki::shared_ref<display_wrapper> display,
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	) :
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
#if defined(RUISAPP_RENDER_OPENGL) || defined(RUISAPP_RENDER_OPENGLES)
		egl_config(
			this->display.get().egl_display, //
			gl_version,
			window_params
		),
		egl_context(
			this->display.get().egl_display, //
			gl_version,
			this->egl_config,
			shared_gl_context_native_window ? shared_gl_context_native_window->egl_context.context : EGL_NO_CONTEXT
		),
#endif
		cur_window_dims(window_params.dims)
	{
		utki::log_debug([](auto& o) {
			o << "native_window constructed" << std::endl;
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

#ifndef RUISAPP_RENDER_VULKAN
	void create_egl_surface()
	{
		utki::assert(!this->is_egl_surface_created(), [](auto& o) {
			o << "create_egl_surface() must not be called more than once";
		});

		this->egl_surface.emplace(
			this->display.get().egl_display, //
			this->egl_config,
			this->wayland_egl_window.window
		);

		// The rendering context was made current with EGL_NO_SURFACE in its constructor,
		// before the surface existed. Now that the surface exists, re-bind the context to
		// it, so that rendering (and the initial eglSwapBuffers) actually reaches the
		// window's buffer. Without this, context::apply() would skip the bind (the context
		// is already "current" on the context stack) and the swap would fail.
		this->bind_rendering_context();
	}

	bool is_egl_surface_created() const noexcept
	{
		return this->egl_surface.has_value();
	}
#endif

	void swap_frame_buffers() override
	{
#ifdef RUISAPP_RENDER_VULKAN
		// For Vulkan, the present is handled by the Vulkan context
		// through the present_func callback.
		if (this->present_func) {
			this->present_func();
		}
#else
		if (this->egl_surface.has_value()) {
			this->egl_surface.value().swap_frame_buffers();
		}
#endif
	}

	r4::vector2<unsigned> get_dims() const noexcept override
	{
		return this->cur_window_dims * this->scale_and_dpi.scale;
	}

	void bind_rendering_context() override
	{
#ifdef RUISAPP_RENDER_VULKAN
		// For Vulkan, there's no concept of binding a rendering context.
		// The Vulkan context is thread-safe and doesn't need to be made current.
#else
		auto& egl_display = this->display.get().egl_display;

		if (this->egl_surface.has_value()) {
			if (eglMakeCurrent(
					egl_display.display,
					this->egl_surface.value().surface,
					this->egl_surface.value().surface,
					this->egl_context.context
				) == EGL_FALSE)
			{
				throw std::runtime_error("eglMakeCurrent() failed");
			}
		} else {
			if (egl_display.extensions.get(egl::extension::khr_surfaceless_context)) {
				eglMakeCurrent(
					egl_display.display, //
					EGL_NO_SURFACE,
					EGL_NO_SURFACE,
					this->egl_context.context
				);
			} else {
				// KHR_surfaceless_context EGL extension is not available, create a dummy pbuffer surface to make the context current
				if (!this->egl_dummy_surface.has_value()) {
					this->egl_dummy_surface.emplace(
						egl_display, //
						this->egl_config
					);
				}
				eglMakeCurrent(
					egl_display.display, //
					this->egl_dummy_surface.value().surface,
					this->egl_dummy_surface.value().surface,
					this->egl_context.context
				);
			}
		}
#endif
	}

	bool is_rendering_context_bound() const noexcept override
	{
#ifdef RUISAPP_RENDER_VULKAN
		// For Vulkan, the context is always "bound" (thread-safe)
		return true;
#else
		return eglGetCurrentContext() == this->egl_context.context;
#endif
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

private:
	ruis::mouse_cursor cur_mouse_cursor = ruis::mouse_cursor::arrow;
	bool mouse_cursor_visible = true;

public:
#ifdef RUISAPP_RENDER_VULKAN
	// Callback for presenting the Vulkan frame. Set by the application glue
	// (see application.cxx make_window) because the native window is graphics-agnostic.
	std::function<void()> present_func;
#endif

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

	void set_vsync_enabled_internal(bool enabled) override
	{
#ifdef RUISAPP_RENDER_VULKAN
		(void)enabled;
		// VSync is handled by the Vulkan present mode
#elif defined(RUISAPP_RENDER_OPENGL) || defined(RUISAPP_RENDER_OPENGLES)
		utki::assert(
			[this]() {
				return this->is_rendering_context_bound();
			},
			SL
		);

		this->egl_context.set_vsync_enabled(enabled);
#endif
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
