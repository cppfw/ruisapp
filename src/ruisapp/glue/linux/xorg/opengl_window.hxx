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

// The OpenGL/OpenGL ES native window. Inherits from xorg_window (the
// graphics-agnostic base) and adds the GL-specific parts: FBConfig/visual
// selection, GL context creation, swap_frame_buffers, bind_rendering_context,
// etc.
//
// The gl_visual_info base class holds the computed FBConfig + X11 visual. It is
// inherited before xorg_window so that the base class can use the visual when
// creating the X11 window (base classes are initialized before members, so the
// visual cannot be a member of the derived class).

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#if defined(RUISAPP_RENDER_OPENGL)
#	include <GL/glx.h>
#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include "../../egl_utils.hxx"
#endif

#include "xorg_window.hxx"

namespace {

// Holds the computed FBConfig + X11 visual. This is a base class of native_window
// (inherited before xorg_window) so that the visual is available when the
// xorg_window base creates the X11 window.
class gl_visual_info
{
public:
#if defined(RUISAPP_RENDER_OPENGL)
	GLXFBConfig fb_config;
	XVisualInfo* const visual_info;
	xorg_visual xvisual;

	gl_visual_info(
		display_wrapper& display, //
		const utki::version_duplet&,
		const ruisapp::window_parameters& window_params
	) :
		fb_config([&]() {
			GLXFBConfig best_fb_config = nullptr;
			std::vector<int> visual_attribs;
			visual_attribs.push_back(GLX_X_RENDERABLE);
			visual_attribs.push_back(True);
			visual_attribs.push_back(GLX_X_VISUAL_TYPE);
			visual_attribs.push_back(GLX_TRUE_COLOR);
			visual_attribs.push_back(GLX_DRAWABLE_TYPE);
			visual_attribs.push_back(GLX_WINDOW_BIT);
			visual_attribs.push_back(GLX_RENDER_TYPE);
			visual_attribs.push_back(GLX_RGBA_BIT);
			visual_attribs.push_back(GLX_DOUBLEBUFFER);
			visual_attribs.push_back(True);
			visual_attribs.push_back(GLX_RED_SIZE);
			visual_attribs.push_back(utki::byte_bits);
			visual_attribs.push_back(GLX_GREEN_SIZE);
			visual_attribs.push_back(utki::byte_bits);
			visual_attribs.push_back(GLX_BLUE_SIZE);
			visual_attribs.push_back(utki::byte_bits);
			visual_attribs.push_back(GLX_ALPHA_SIZE);
			visual_attribs.push_back(utki::byte_bits);

			if (window_params.buffers.get(ruisapp::buffer::depth)) {
				visual_attribs.push_back(GLX_DEPTH_SIZE);
				visual_attribs.push_back(utki::byte_bits * 3); // 24 bits per pixel for depth buffer
			}
			if (window_params.buffers.get(ruisapp::buffer::stencil)) {
				visual_attribs.push_back(GLX_STENCIL_SIZE);
				visual_attribs.push_back(utki::byte_bits);
			}

			visual_attribs.push_back(None);

			utki::span<GLXFBConfig> fb_configs = [&]() {
				int fbcount = 0;
				GLXFBConfig* fb_configs = glXChooseFBConfig(
					display.xorg_display.display,
					display.xorg_display.get_default_screen(),
					visual_attribs.data(),
					&fbcount
				);
				if (!fb_configs) {
					throw std::runtime_error("glXChooseFBConfig() returned empty list");
				}
				return utki::make_span(fb_configs, fbcount);
			}();

			utki::scope_exit scope_exit_fbc([&fb_configs]() {
				// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
				XFree(fb_configs.data());
			});

			GLXFBConfig worst_fb_config = nullptr;
			int best_num_samp = -1;
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
			int worst_num_samp = 999;

			for (auto fb_config : fb_configs) {
				XVisualInfo* vi = glXGetVisualFromFBConfig(
					display.xorg_display.display, //
					fb_config
				);
				if (!vi) {
					continue;
				}

				// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
				int samp_buf;
				glXGetFBConfigAttrib(
					display.xorg_display.display, //
					fb_config,
					GLX_SAMPLE_BUFFERS,
					&samp_buf
				);

				// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
				int samples;
				glXGetFBConfigAttrib(
					display.xorg_display.display, //
					fb_config,
					GLX_SAMPLES,
					&samples
				);

				if (!best_fb_config || (samp_buf && samples > best_num_samp)) {
					best_fb_config = fb_config;
					best_num_samp = samples;
				}
				if (!worst_fb_config || !samp_buf || samples < worst_num_samp) {
					worst_fb_config = fb_config;
					worst_num_samp = samples;
				}

				XFree(vi);
			}

			utki::assert(best_fb_config, SL);
			return best_fb_config;
		}()),
		visual_info([&]() {
			auto visual_info = glXGetVisualFromFBConfig(
				display.xorg_display.display, //
				this->fb_config
			);
			if (!visual_info) {
				throw std::runtime_error("glXGetVisualFromFBConfig() failed");
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

#elif defined(RUISAPP_RENDER_OPENGLES)
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
#endif
};

#if defined(RUISAPP_RENDER_OPENGL)
// Creates the GLX context
struct glx_context_wrapper {
	display_wrapper& display;

	enum class glx_extension {
		glx_arb_create_context,
		glx_ext_swap_control,
		glx_mesa_swap_control,

		enum_size
	};

	// glXGetProcAddressARB() will return non-null pointer even if extension is
	// not supported, so we need to explicitly check for supported extensions.
	// SOURCE:
	// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/
	const utki::flags<glx_extension> supported_extensions;

	const GLXContext context;

	glx_context_wrapper(
		display_wrapper& display, //
		XVisualInfo* visual_info,
		const utki::version_duplet& gl_version,
		GLXFBConfig fb_config,
		GLXContext shared_glx_context
	) :
		display(display),
		supported_extensions([&]() {
			utki::flags<glx_extension> supported = false;

			auto glx_extensions_string = std::string_view(glXQueryExtensionsString(
				this->display.xorg_display.display, //
				visual_info->screen
			));
			utki::log_debug([&](auto& o) {
				o << "glx_extensions_string = " << glx_extensions_string << std::endl;
			});

			auto glx_extensions = utki::split(glx_extensions_string);

			using namespace std::string_view_literals;

			if (std::ranges::find(
					glx_extensions, //
					"GLX_ARB_create_context"sv
				) != glx_extensions.end())
			{
				supported.set(glx_extension::glx_arb_create_context);
			}

			if (std::ranges::find(
					glx_extensions, //
					"GLX_EXT_swap_control"sv
				) != glx_extensions.end())
			{
				supported.set(glx_extension::glx_ext_swap_control);
			}

			if (std::ranges::find( //
					glx_extensions,
					"GLX_MESA_swap_control"sv
				) != glx_extensions.end())
			{
				supported.set(glx_extension::glx_mesa_swap_control);
			}

			return supported;
		}()),
		context([&]() {
			GLXContext gl_context = nullptr;

			if (this->supported_extensions.get(glx_extension::glx_arb_create_context)) {
				// GLX_ARB_create_context is supported

				// NOTE: glXGetProcAddressARB() is guaranteed to be present in all GLX
				// versions.
				//       glXGetProcAddress() is not guaranteed.
				// SOURCE:
				// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

				auto glx_create_context_attribs_arb = PFNGLXCREATECONTEXTATTRIBSARBPROC(glXGetProcAddressARB(
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
					reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")
				));

				if (!glx_create_context_attribs_arb) {
					// this should not happen since we checked extension presence, and
					// anyway, glXGetProcAddressARB() never returns nullptr according to
					// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/
					// so, this check for null is just in case future version of GLX may
					// return null
					throw std::runtime_error("glXCreateContextAttribsARB() not found");
				}

				auto graphics_api_version = [&ver = gl_version]() {
					if (ver.to_uint32_t() == 0) {
						// default OpenGL version is 2.0
						return utki::version_duplet{
							.major = 2, //
							.minor = 0
						};
					}
					if (ver.major < 2) {
						throw std::invalid_argument(
							utki::cat("minimal supported OpenGL version is 2.0, requested: ", ver)
						);
					}
					return ver;
				}();

				static const std::array<int, 7> context_attribs = {
					GLX_CONTEXT_MAJOR_VERSION_ARB,
					graphics_api_version.major,
					GLX_CONTEXT_MINOR_VERSION_ARB,
					graphics_api_version.minor,
					GLX_CONTEXT_PROFILE_MASK_ARB,
					// we don't need compatibility context
					GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
					None
				};

				gl_context = glx_create_context_attribs_arb(
					this->display.xorg_display.display, //
					fb_config,
					shared_glx_context,
					GL_TRUE,
					context_attribs.data()
				);
			} else {
				// GLX_ARB_create_context is not supported
				gl_context = glXCreateContext(
					this->display.xorg_display.display, //
					visual_info,
					shared_glx_context,
					GL_TRUE
				);
			}

			// sync to ensure any errors generated are processed
			XSync(
				this->display.xorg_display.display, //
				False
			);

			if (gl_context == nullptr) {
				throw std::runtime_error("glXCreateContext() failed");
			}

			return gl_context;
		}())
	{}

	glx_context_wrapper(const glx_context_wrapper&) = delete;
	glx_context_wrapper& operator=(const glx_context_wrapper&) = delete;

	glx_context_wrapper(glx_context_wrapper&&) = delete;
	glx_context_wrapper& operator=(glx_context_wrapper&&) = delete;

	~glx_context_wrapper()
	{
		if (glXGetCurrentContext() == this->context) {
			glXMakeCurrent(
				this->display.xorg_display.display, //
				None,
				nullptr
			);
		}
		glXDestroyContext(
			this->display.xorg_display.display, //
			this->context
		);
	}
};
#endif

class native_window : public gl_visual_info, public xorg_window
{
public:
	native_window(
		utki::shared_ref<display_wrapper> display, //
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		const native_window* shared_gl_context_native_window
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
#if defined(RUISAPP_RENDER_OPENGL)
		glx_context(
			this->display.get(), //
			this->visual_info,
			gl_version,
			this->fb_config,
			shared_gl_context_native_window ? shared_gl_context_native_window->glx_context.context : nullptr
		)
#elif defined(RUISAPP_RENDER_OPENGLES)
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
#endif
	{
#if defined(RUISAPP_RENDER_OPENGL)
		// if there is no any GL context current, then set this one before calling glewInit()
		if (glXGetCurrentContext() == nullptr) {
			this->bind_rendering_context();
		}
		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("GLEW initialization failed");
		}
#endif
	}

	native_window(const native_window&) = delete;
	native_window& operator=(const native_window&) = delete;

	native_window(native_window&&) = delete;
	native_window& operator=(native_window&&) = delete;

	~native_window() override = default;

	void bind_rendering_context() override
	{
#ifdef RUISAPP_RENDER_OPENGL
		glXMakeCurrent(
			this->display.get().xorg_display.display, //
			this->window.window,
			this->glx_context.context
		);
#elif defined(RUISAPP_RENDER_OPENGLES)
		if (eglMakeCurrent(
				this->display.get().egl_display.display,
				this->egl_surface.surface,
				this->egl_surface.surface,
				this->egl_context.context
			) == EGL_FALSE)
		{
			throw std::runtime_error("eglMakeCurrent() failed");
		}
#endif
	}

	bool is_rendering_context_bound() const noexcept override
	{
#ifdef RUISAPP_RENDER_OPENGL
		return glXGetCurrentContext() == this->glx_context.context;
#elif defined(RUISAPP_RENDER_OPENGLES)
		return eglGetCurrentContext() == this->egl_context.context;
#endif
	}

	void swap_frame_buffers() override
	{
#ifdef RUISAPP_RENDER_OPENGL
		glXSwapBuffers(
			this->display.get().xorg_display.display, //
			this->window.window
		);
		if (this->is_vsync_enabled()) {
			// With glFinish() call here it works better when VSYNC is enabled.
			// This is noticable when an app renders a mouse cursor itself,
			// the cursor moves with less latency.
			// For some reason, the effect is only present if calling the glFinish()
			// right after swapping buffers, if calling it before swapping buffers,
			// the effect is not present.
			glFinish();
		}
#elif defined(RUISAPP_RENDER_OPENGLES)
		this->egl_surface.swap_frame_buffers();
#else
#	error "Unknown graphics API"
#endif
	}

	void set_vsync_enabled_internal(bool enable) override
	{
		utki::assert(
			[this]() {
				return this->is_rendering_context_bound();
			},
			SL
		);

#ifdef RUISAPP_RENDER_OPENGL
		// disable v-sync via swap control extension
		if (this->glx_context.supported_extensions.get(glx_context_wrapper::glx_extension::glx_ext_swap_control)) {
			utki::log_debug([](auto& o) {
				o << "GLX_EXT_swap_control is supported\n";
			});

			auto glx_swap_interval_ext = PFNGLXSWAPINTERVALEXTPROC(glXGetProcAddressARB(
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				reinterpret_cast<const GLubyte*>("glXSwapIntervalEXT")
			));

			utki::assert(glx_swap_interval_ext, SL);

			// disable v-sync
			glx_swap_interval_ext(
				this->display.get().xorg_display.display, //
				this->window.window,
				enable ? 1 : 0 // swap interval in vsync frames
			);
		} else if (this->glx_context.supported_extensions.get(glx_context_wrapper::glx_extension::glx_mesa_swap_control
				   ))
		{
			utki::log_debug([](auto& o) {
				o << "GLX_MESA_swap_control is supported\n";
			});

			auto glx_swap_interval_mesa = PFNGLXSWAPINTERVALMESAPROC(glXGetProcAddressARB(
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				reinterpret_cast<const GLubyte*>("glXSwapIntervalMESA")
			));

			utki::assert(glx_swap_interval_mesa, SL);

			// disable v-sync
			if (glx_swap_interval_mesa(
					enable ? 1 : 0 // swap interval in vsync frames
				) != 0)
			{
				utki::logcat("WARNING: glXSwapIntervalMESA(", enable, ") failed");
			}
		} else {
			std::cout << "none of GLX_EXT_swap_control, GLX_MESA_swap_control GLX "
					  << "extensions are supported. Not disabling v-sync." << std::endl;
		}

		// sync to ensure any errors generated are processed
		XSync(
			this->display.get().xorg_display.display, //
			False
		);
#elif defined(RUISAPP_RENDER_OPENGLES)
		utki::logcat_debug("eglSwapInterval(", enable ? "true" : "false", ")\n");
		if (eglSwapInterval(
				this->display.get().egl_display.display, //
				enable ? 1 : 0 // swap interval in vsync frames
			) != EGL_TRUE)
		{
			utki::logcat("WARNING: eglSwapInterval(", enable, ") failed");
		}
#endif
	}

private:
#if defined(RUISAPP_RENDER_OPENGL)
	const glx_context_wrapper glx_context;
#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_surface_wrapper egl_surface;
	egl_context_wrapper egl_context;
#endif
};

} // namespace
