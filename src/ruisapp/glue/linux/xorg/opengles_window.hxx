// this file is part of ruisapp application framework
//
// this file is licensed under the terms of the MIT license, which can be found in the LICENSE file
//
// SPDX-License-Identifier: MIT

// OpenGL ES-specific native window implementation.
// The gl_visual_info base class holds the computed EGLConfig + X11 visual. It is
// inherited before xorg_window so that the base class can use the visual when
// creating the X11 window (base classes are initialized before members, so the
// visual cannot be a member of the derived class).

#include <EGL/egl.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "../../egl_utils.hxx"

#include "xorg_window.hxx"

namespace {

// Holds the computed EGLConfig + X11 visual. This is a base class of native_window
// (inherited before xorg_window) so that the visual is available when the
// xorg_window base creates the X11 window.
class gl_visual_info
{
public:
	const egl_config_wrapper egl_config;
	XVisualInfo* const visual_info;
	xorg_visual xvisual;

	gl_visual_info(
		display_wrapper& display, //
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params
	) :
		egl_config(
			display.egl_display, //
			gl_version,
			window_params
		),
		visual_info([&]() {
			EGLint vid = 0;

			if (!eglGetConfigAttrib(
					display.egl_display.display, //
					this->egl_config.config,
					EGL_NATIVE_VISUAL_ID,
					&vid
				))
			{
				throw std::runtime_error("eglGetConfigAttrib() failed");
			}

			int num_visuals = 0;
			XVisualInfo vis_template;
			vis_template.visualid = vid;
			auto visual_info = XGetVisualInfo(
				display.xorg_display.display, //
				VisualIDMask,
				&vis_template,
				&num_visuals
			);
			if (!visual_info) {
				throw std::runtime_error("XGetVisualInfo() failed");
			}
			return visual_info;
		}()),
		xvisual([this]() {
			return xorg_visual{
				.visual = this->visual_info->visual, //
				.depth = unsigned(this->visual_info->depth)
			};
		}())
	{}

	gl_visual_info(const gl_visual_info&) = delete;
	gl_visual_info& operator=(const gl_visual_info&) = delete;

	gl_visual_info(gl_visual_info&&) = delete;
	gl_visual_info& operator=(gl_visual_info&&) = delete;

	~gl_visual_info()
	{
		XFree(this->visual_info);
	}
};

class opengles_native_window : public gl_visual_info, public xorg_window
{
public:
	opengles_native_window(
		utki::shared_ref<display_wrapper> display, //
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		opengles_native_window* shared_gl_context_native_window
	) :
		gl_visual_info(
			display.get(), //
			gl_version,
			window_params
		),
		xorg_window(
			display, //
			window_params,
			shared_gl_context_native_window,
			this->xvisual
		),
		egl_surface(
			this->display.get().egl_display, //
			this->egl_config,
			this->window.window
		),
		egl_context(
			this->display.get().egl_display, //
			gl_version,
			this->egl_config,
			shared_gl_context_native_window ? shared_gl_context_native_window->egl_context.context : EGL_NO_CONTEXT
		)
	{}

	opengles_native_window(const opengles_native_window&) = delete;
	opengles_native_window& operator=(const opengles_native_window&) = delete;

	opengles_native_window(opengles_native_window&&) = delete;
	opengles_native_window& operator=(opengles_native_window&&) = delete;

	~opengles_native_window() override = default;

	void bind_rendering_context() override
	{
		if (eglMakeCurrent(
				this->display.get().egl_display.display,
				this->egl_surface.surface,
				this->egl_surface.surface,
				this->egl_context.context
			) == EGL_FALSE)
		{
			throw std::runtime_error("eglMakeCurrent() failed");
		}
	}

	bool is_rendering_context_bound() const noexcept override
	{
		return eglGetCurrentContext() == this->egl_context.context;
	}

	void swap_frame_buffers() override
	{
		this->egl_surface.swap_frame_buffers();
	}

	void set_vsync_enabled_internal(bool enable) override
	{
		utki::assert(
			[this]() {
				return this->is_rendering_context_bound();
			},
			SL
		);

		utki::logcat_debug("eglSwapInterval(", enable ? "true" : "false", ")\n");
		if (eglSwapInterval(
				this->display.get().egl_display.display, //
				enable ? 1 : 0 // swap interval in vsync frames
			) != EGL_TRUE)
		{
			utki::logcat("WARNING: eglSwapInterval(", enable, ") failed");
		}
	}

private:
	egl_surface_wrapper egl_surface;
	egl_context_wrapper egl_context;
};

} // namespace
