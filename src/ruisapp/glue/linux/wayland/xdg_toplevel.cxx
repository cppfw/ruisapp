#include "xdg_toplevel.hxx"

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

	xdg_toplevel_set_title(
		this->toplevel, //
		window_params.title.c_str()
	);
	utki::logcat_debug("xdg_toplevel_wrapper::xdg_toplevel_wrapper(): add listener", '\n');
	xdg_toplevel_add_listener(
		this->toplevel, //
		&listener,
		this
	);
	utki::logcat_debug("xdg_toplevel_wrapper::xdg_toplevel_wrapper(): listener added, commit wayland surface", '\n');

	wayland_surface.commit();

	utki::logcat_debug("xdg_toplevel_wrapper::xdg_toplevel_wrapper(): wayland surface committed", '\n');
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
		o << "window configure" << std::endl;
	});

	utki::log_debug([&](auto& o) {
		o << "  width = " << std::dec << width << ", height = " << height << std::endl;
	});

	bool fullscreen = false;

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

	// TODO: is needed?
	// if (!ruisapp::application::is_constructed()) {
	// 	// unable to obtain window object before application is constructed,
	// 	// cannot do more without window object
	// 	utki::log_debug([](auto& o) {
	// 		o << "  called within application constructor" << std::endl;
	// 	});
	// 	return;
	// }

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

	// if both width and height are zero, then it is one of states checked above
	if (width == 0 && height == 0) {
		if (win.is_actually_fullscreen != fullscreen) {
			if (!fullscreen) {
				// exited fullscreen mode
				win.resize(natwin.pre_fullscreen_win_dims);
			}
			win.is_actually_fullscreen = fullscreen;
		}
		return;
	}

	utki::assert(width >= 0, SL);
	utki::assert(height >= 0, SL);

	win.is_actually_fullscreen = fullscreen;

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
