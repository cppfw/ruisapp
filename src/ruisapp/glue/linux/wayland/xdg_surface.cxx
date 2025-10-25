#include "xdg_surface.hxx"

#include "application.hxx"

xdg_surface_wrapper::xdg_surface_wrapper(
	wayland_surface_wrapper& wayland_surface, //
	xdg_wm_base_wrapper& xdg_wm_base
) :
	wayland_surface(wayland_surface),
	surface(xdg_wm_base_get_xdg_surface(
		xdg_wm_base.wm_base, //
		wayland_surface.surface
	))
{
	if (!this->surface) {
		throw std::runtime_error("could not create wayland xdg surface");
	}

	utki::log_debug([&](auto& o) {
		auto id = wl_proxy_get_id(reinterpret_cast<wl_proxy*>(this->surface));
		o << "xgd_surface: CREATED, id = " << id << std::endl;
	});

	xdg_surface_add_listener(
		this->surface, //
		&listener,
		this // user data
	);
}

void xdg_surface_wrapper::xdg_surface_configure(
	void* data, //
	xdg_surface* surface,
	uint32_t serial
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<xdg_surface_wrapper*>(data);

	utki::log_debug([&](auto& o) {
		auto id = wl_proxy_get_id(reinterpret_cast<wl_proxy*>(surface));
		o << "xgd_surface: CONFIGURE for surface id = " << id << std::endl;
	});

	// Wayland protocol requires to acknowledge the configure event
	xdg_surface_ack_configure(
		surface, //
		serial
	);

	// The configure callback is invoked by Wayland after initial toplevel configure has been invoked.
	// Right after creating a Wayland surface, according to the Wayland protocol, it
	// should do the initial configure call, after which one is supposed to do the
	// initial surface commit. In case of EGL we have to call eglSwapBuffers(), which will do the
	// surface commit for us. This makes the surface to be mapped to the screen and become visible.
	// If this sequence is not honored the surface will not appear on the screen.

	auto& glue = get_glue();

	auto window = glue.get_window(self.wayland_surface.surface);
	if (!window) {
		utki::logcat_debug("  could not find window object, perhaps called for shared gl context window", '\n');
		utki::assert(self.wayland_surface.surface == glue.get_shared_gl_context_window_id(), SL);
		return;
	}

	auto& win = *window;
	auto& natwin = win.ruis_native_window.get();

	natwin.create_egl_surface();

	// swap EGL frame buffers with the window's EGL context made current
	win.gui.context.get().ren().ctx().apply([&]() {
		natwin.swap_frame_buffers();
	});

	// self.wayland_surface.commit();
}
