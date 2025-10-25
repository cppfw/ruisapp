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

#include "xdg_toplevel.hxx"

#include "application.hxx"

xdg_toplevel_wrapper::xdg_toplevel_wrapper(
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

	// utki::logcat_debug("xdg_toplevel_wrapper::xdg_toplevel_wrapper(): add listener", '\n');
	xdg_toplevel_add_listener(
		this->toplevel, //
		&listener,
		this
	);
	// utki::logcat_debug("xdg_toplevel_wrapper::xdg_toplevel_wrapper(): listener added", '\n');

	xdg_toplevel_set_title(
		this->toplevel, //
		window_params.title.c_str()
	);

	wayland_surface.commit();

	// utki::logcat_debug("xdg_toplevel_wrapper::xdg_toplevel_wrapper(): wayland surface committed", '\n');
}

void xdg_toplevel_wrapper::xdg_toplevel_configure(
	void* data, //
	xdg_toplevel* xdg_toplevel,
	int32_t width,
	int32_t height,
	wl_array* states
)
{
	utki::log_debug([](auto& o) {
		o << "toplevel CONFIGURE" << std::endl;
	});

	utki::log_debug([&](auto& o) {
		o << "  width = " << std::dec << width << ", height = " << height << std::endl;
	});

	app_window::window_state state;

	utki::assert(states, SL);
	utki::assert(states->size % sizeof(uint32_t) == 0, SL);
	auto states_span = utki::make_span(
		static_cast<uint32_t*>(states->data), //
		states->size / sizeof(uint32_t)
	);

	utki::log_debug([&](auto& o) {
		o << "  states(" << states_span.size() << "):" << std::endl;
	});
	for (uint32_t s : states_span) {
		switch (s) {
			case XDG_TOPLEVEL_STATE_FULLSCREEN:
				utki::log_debug([](auto& o) {
					o << "    fullscreen" << std::endl;
				});
				state.fullscreen = true;
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

									// TODO: these are not supported in Debian bookworm and Ubuntu Noble, though supported in Debian trixie. Enagle these when support becomes more common.
									// case XDG_TOPLEVEL_STATE_SUSPENDED:
									// 	return "suspended";
									// case XDG_TOPLEVEL_STATE_CONSTRAINED_LEFT:
									// 	return "constrained left";
									// case XDG_TOPLEVEL_STATE_CONSTRAINED_RIGHT:
									// 	return "constrained right";
									// case XDG_TOPLEVEL_STATE_CONSTRAINED_TOP:
									// 	return "constrained top";
									// case XDG_TOPLEVEL_STATE_CONSTRAINED_BOTTOM:
									// 	return "constrained bottom";

								default:
									return "unknown";
							}
						}()
					  << std::endl;
				});
				break;
		}
	}

	utki::assert(data, SL);
	auto& self = *static_cast<xdg_toplevel_wrapper*>(data);

	auto& glue = get_glue();

	auto window = glue.get_window(self.wayland_surface.surface);
	if (!window) {
		utki::logcat_debug("  could not find window object, perhaps called for shared gl context window", '\n');
		utki::assert(self.wayland_surface.surface == glue.get_shared_gl_context_window_id(), SL);
		return;
	}
	auto& win = *window;
	auto& natwin = win.ruis_native_window.get();

	utki::logcat_debug("  window sequence_number: ", natwin.sequence_number, '\n');

	utki::scope_exit save_actual_state_scope_exit([&]() {
		win.actual_state = state;
	});

	// TODO: refactor. Figure out what is the actual protocol about configure calls with zero/non-zero window size,
	// states reported etc.

	// if both width and height are zero, then it is a state change
	if (width == 0 && height == 0) {
		if (states_span.empty()) {
			utki::logcat_debug("  initial configure call", '\n');

			self.wayland_surface.commit();
			return;
		}

		if (win.actual_state.fullscreen != state.fullscreen) {
			if (!state.fullscreen) {
				// exited fullscreen mode
				win.resize(natwin.pre_fullscreen_win_dims);
			} else {
				// entered fullscreen mode
				utki::logcat_debug(
					"xdg_toplevel_wrapper::xdg_toplevel_configure(): window(", //
					natwin.sequence_number,
					") ",
					"enter fullscreen, cur_window_dims = ",
					natwin.cur_window_dims
				);
			}
		}

		return;
	}

	utki::assert(width >= 0, SL);
	utki::assert(height >= 0, SL);

	win.resize(
		{uint32_t(width), //
		 uint32_t(height)}
	);
}

void xdg_toplevel_wrapper::xdg_toplevel_close(
	void* data, //
	xdg_toplevel* xdg_toplevel
)
{
	// user requested to close the window

	utki::assert(data, SL);
	auto& self = *static_cast<xdg_toplevel_wrapper*>(data);

	auto& glue = get_glue();

	auto window = glue.get_window(self.wayland_surface.surface);
	if (!window) {
		utki::logcat_debug("xdg_toplevel_wrapper::xdg_toplevel_close(): could not find window object", '\n');
		utki::assert(false, SL);
		return;
	}

	auto& natwin = window->ruis_native_window.get();

	if (natwin.close_handler) {
		natwin.close_handler();
	}
}
