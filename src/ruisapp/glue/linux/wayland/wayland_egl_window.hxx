#pragma once

#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#include "wayland_surface.hxx"

namespace {
struct wayland_egl_window_wrapper {
	wl_egl_window* const window;

	wayland_egl_window_wrapper(
		wayland_surface_wrapper& wayland_surface, //
		r4::vector2<unsigned> dims
	) :
		window([&]() {
			auto int_dims = dims.to<int>();

			// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
			auto w = wl_egl_window_create(
				wayland_surface.surface, //
				int_dims.x(),
				int_dims.y()
			);

			if (!w) {
				throw std::runtime_error("could not create wayland egl window");
			}
			return w;
		}())
	{}

	wayland_egl_window_wrapper(const wayland_egl_window_wrapper&) = delete;
	wayland_egl_window_wrapper& operator=(const wayland_egl_window_wrapper&) = delete;

	wayland_egl_window_wrapper(wayland_egl_window_wrapper&&) = delete;
	wayland_egl_window_wrapper& operator=(wayland_egl_window_wrapper&&) = delete;

	~wayland_egl_window_wrapper()
	{
		wl_egl_window_destroy(this->window);
	}
};
} // namespace
