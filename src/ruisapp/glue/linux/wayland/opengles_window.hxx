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

#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#include "../../egl_utils.hxx"

#include "wayland_window.hxx"

namespace {
// OpenGL ES-specific native window for Wayland.
class native_window : public wayland_window
{
	egl_config_wrapper egl_config;
	egl_context_wrapper egl_context;
	// An offscreen pbuffer surface used as a fallback for the shared GL context
	// when no real window surface is available (e.g. during resource loading).
	std::optional<egl_pbuffer_surface_wrapper> egl_dummy_surface;
	std::optional<egl_surface_wrapper> egl_surface;

public:
	native_window(
		utki::shared_ref<display_wrapper> display,
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	) :
		wayland_window(
			std::move(display), //
			window_params
		),
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
		)
	{}

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

	void swap_frame_buffers() override
	{
		if (this->egl_surface.has_value()) {
			this->egl_surface.value().swap_frame_buffers();
		}
	}

	void bind_rendering_context() override
	{
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
	}

	bool is_rendering_context_bound() const noexcept override
	{
		return eglGetCurrentContext() == this->egl_context.context;
	}

	void set_vsync_enabled_internal(bool enabled) override
	{
		utki::assert(
			[this]() {
				return this->is_rendering_context_bound();
			},
			SL
		);

		this->egl_context.set_vsync_enabled(enabled);
	}
};
} // namespace
