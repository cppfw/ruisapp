#pragma once

#include <utki/debug.hpp>
#include <utki/span.hpp>
#include <xdg-shell-client-protocol.h>

#include "wayland_surface.hxx"
#include "xdg_surface.hxx"

namespace {
struct xdg_toplevel_wrapper {
	wayland_surface_wrapper& wayland_surface;

	xdg_toplevel* toplevel;

	static void xdg_toplevel_configure(
		void* data, //
		xdg_toplevel* xdg_toplevel,
		int32_t width,
		int32_t height,
		wl_array* states
	);

	static void xdg_toplevel_close(
		void* data, //
		xdg_toplevel* xdg_toplevel
	);

	constexpr static const xdg_toplevel_listener listener = {
		.configure = xdg_toplevel_configure,
		.close = &xdg_toplevel_close,
	};

	xdg_toplevel_wrapper(
		wayland_surface_wrapper& wayland_surface, //
		xdg_surface_wrapper& xdg_surface,
		const ruisapp::window_parameters& window_params
	) :
		wayland_surface(wayland_surface),
		toplevel(xdg_surface_get_toplevel(xdg_surface.surface))
	{
		if (!this->toplevel) {
			throw std::runtime_error("could not get wayland xdg toplevel");
		}

		xdg_toplevel_set_title(
			this->toplevel, //
			window_params.title.c_str()
		);
		xdg_toplevel_add_listener(
			this->toplevel, //
			&listener,
			this
		);

		wayland_surface.commit();
	}

	xdg_toplevel_wrapper(const xdg_toplevel_wrapper&) = delete;
	xdg_toplevel_wrapper& operator=(const xdg_toplevel_wrapper&) = delete;

	xdg_toplevel_wrapper(xdg_toplevel_wrapper&&) = delete;
	xdg_toplevel_wrapper& operator=(xdg_toplevel_wrapper&&) = delete;

	~xdg_toplevel_wrapper()
	{
		xdg_toplevel_destroy(this->toplevel);
	}
};
} // namespace
