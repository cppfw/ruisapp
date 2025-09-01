#pragma once

#include <ruis/render/native_window.hpp>
#include <ruisapp/window.hpp>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#include "../../egl_utils.hxx"

#include "display.hxx"
#include "wayland_egl_window.hxx"
#include "wayland_surface.hxx"
#include "xdg_surface.hxx"
#include "xdg_toplevel.hxx"

namespace {
class native_window : public ruis::render::native_window
{
	utki::shared_ref<display_wrapper> display;

	wayland_surface_wrapper wayland_surface;
	xdg_surface_wrapper xdg_surface;
	xdg_toplevel_wrapper xdg_toplevel;
	wayland_egl_window_wrapper wayland_egl_window;

	egl_config_wrapper egl_config;
	egl_surface_wrapper egl_surface;
	egl_context_wrapper egl_context;

	// keep track of current window dimensions
	r4::vector2<uint32_t> cur_window_dims;

	bool fullscreen = false;
	r4::vector2<uint32_t> pre_fullscreen_win_dims;

	ruis::real scale = 1;

	bool outputs_changed_message_pending = false;

public:
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
		egl_config(
			this->display.get().egl_display, //
			gl_version,
			window_params
		),
		egl_surface(
			this->display.get().egl_display, //
			this->egl_config,
			this->wayland_egl_window.window
		),
		egl_context(
			this->display.get().egl_display, //
			gl_version,
			this->egl_config,
			shared_gl_context_native_window ? shared_gl_context_native_window->egl_context.context : EGL_NO_CONTEXT
		)
	{
		// WORKAROUND: the following calls are figured out by trial and error. Without those the wayland main loop
		//             either gets stuck on waiting for events and no events come and window is not shown, or
		//             some call related to wayland events queue fails with error.
		// no idea why roundtrip is needed, perhaps to configure the xdg surface before actually drawing to it
		wl_display_roundtrip(this->display.get().wayland_display.display);
		// no idea why initial buffer swap is needed, perhaps it moves the window configure procedure forward somehow
		this->swap_frame_buffers();

		utki::log_debug([](auto& o) {
			o << "native_window constructed" << std::endl;
		});
	}

	void swap_frame_buffers() override
	{
		eglSwapBuffers(
			this->display.get().egl_display.display, //
			this->egl_surface.surface
		);
	}

	void resize(r4::vector2<uint32_t> dims)
	{
		this->cur_window_dims = dims;

		utki::log_debug([&](auto& o) {
			o << "resize window to " << std::dec << dims << std::endl;
		});

		auto sd = this->wayland_surface.find_scale_and_dpi(this->display.get().wayland_registry.outputs);

		dims *= sd.scale;

		wl_egl_window_resize(
			this->wayland_egl_window.window, //
			int(dims.x()),
			int(dims.y()),
			0,
			0
		);

		wayland_region_wrapper region(this->display.get().wayland_compositor);
		region.add(r4::rectangle(
			{0, 0}, //
			dims.to<int32_t>()
		));

		this->wayland_surface.set_opaque_region(region);

		this->wayland_surface.set_buffer_scale(sd.scale);

		this->wayland_surface.commit();

		utki::log_debug([&](auto& o) {
			o << "final window scale = " << sd.scale << std::endl;
		});

		this->scale = ruis::real(sd.scale);

		auto& app = ruisapp::inst();

		// update ruis::context::units
		auto& units = app.gui.context.get().units;
		units.set_dots_per_pp(this->scale);
		units.set_dots_per_inch(ruis::real(sd.dpi));

		update_window_rect(app, ruis::rect(0, dims.to<ruis::real>()));
	}

	void notify_outputs_changed()
	{
		if (this->outputs_changed_message_pending) {
			return;
		}

		this->outputs_changed_message_pending = true;

		this->ui_queue.push_back([this]() {
			this->outputs_changed_message_pending = false;

			// this call will update ruis::context::units values
			this->resize(this->cur_window_dims);

			// reload widgets herarchy due to possible update of ruis::context::units values
			auto& app = ruisapp::application::inst();
			auto& root_widget = app.gui.get_root();
			root_widget.reload();
		});
	}
};
} // namespace
