#pragma once

#include <ruisapp/window.hpp>
#include <utki/debug.hpp>
#include <utki/span.hpp>
#include <xdg-shell-client-protocol.h>

#include "wayland_surface.hxx"
#include "xdg_surface.hxx"

namespace {
struct xdg_toplevel_wrapper {
	xdg_toplevel* toplevel;

	static void xdg_toplevel_configure(
		void* data, //
		xdg_toplevel* xdg_toplevel,
		int32_t width,
		int32_t height,
		wl_array* states
	)
	{
		utki::log_debug([](auto& o) {
			o << "window configure" << std::endl;
		});

		utki::log_debug([&](auto& o) {
			o << "  width = " << std::dec << width << ", height = " << height << std::endl;
		});

		bool fullscreen = false;

		utki::log_debug([&](auto& o) {
			o << "  states:" << std::endl;
		});
		utki::assert(states, SL);
		utki::assert(states->size % sizeof(uint32_t) == 0, SL);
		for (uint32_t s : utki::make_span(
				 static_cast<uint32_t*>(states->data), //
				 states->size / sizeof(uint32_t)
			 ))
		{
			switch (s) {
				case XDG_TOPLEVEL_STATE_FULLSCREEN:
					utki::log_debug([](auto& o) {
						o << "    fullscreen" << std::endl;
					});
					fullscreen = true;
					break;
				default:
					utki::log_debug([&](auto& o) {
						o << "    " <<
							[&s]() {
								switch (s) {
									case XDG_TOPLEVEL_STATE_MAXIMIZED:
										return "maximized";
									case XDG_TOPLEVEL_STATE_RESIZING:
										return "resizing";
									case XDG_TOPLEVEL_STATE_ACTIVATED:
										return "activated";
									case XDG_TOPLEVEL_STATE_TILED_LEFT:
										return "tiled left";
									case XDG_TOPLEVEL_STATE_TILED_RIGHT:
										return "tiled right";
									case XDG_TOPLEVEL_STATE_TILED_TOP:
										return "tiled top";
									case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
										return "tiled bottom";
									default:
										return "unknown";
								}
							}()
						  << std::endl;
					});
					break;
			}
		}

		if (!application_constructed) {
			// unable to obtain window_wrapper object before application is constructed,
			// cannot do more without window_wrapper object
			utki::log_debug([](auto& o) {
				o << "  called within application constructor" << std::endl;
			});
			return;
		}

		auto& ww = get_impl(ruisapp::application::inst());

		// if both width and height are zero, then it is one of states checked above
		if (width == 0 && height == 0) {
			if (ww.fullscreen != fullscreen) {
				if (!fullscreen) {
					// exited fullscreen mode
					ww.resize(ww.pre_fullscreen_win_dims);
				}
			}
			ww.fullscreen = fullscreen;
			return;
		}

		utki::assert(width >= 0, SL);
		utki::assert(height >= 0, SL);

		ww.fullscreen = fullscreen;

		ww.resize(
			{uint32_t(width), //
			 uint32_t(height)}
		);
	}

	static void xdg_toplevel_close(
		void* data, //
		xdg_toplevel* xdg_toplevel
	)
	{
		// TODO: invoke window close_handler
		// window closed
		auto& ww = get_impl(ruisapp::inst());
		ww.quit_flag.store(true);
	}

	constexpr static const xdg_toplevel_listener listener = {
		.configure = xdg_toplevel_configure,
		.close = &xdg_toplevel_close,
	};

	xdg_toplevel_wrapper(
		wayland_surface_wrapper& surface, //
		xdg_surface_wrapper& xdg_surface,
		const ruisapp::window_parameters& window_params
	) :
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
			nullptr
		);

		surface.commit();
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
