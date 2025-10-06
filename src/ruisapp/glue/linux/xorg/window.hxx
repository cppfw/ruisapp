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

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glx.h>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include "../../egl_utils.hxx"

#else
#	error "Unknown graphics API"
#endif

#include "display.hxx"

namespace {

class native_window : public ruis::render::native_window
{
	utki::shared_ref<display_wrapper> display;

#ifdef RUISAPP_RENDER_OPENGL
	struct fb_config_wrapper {
		GLXFBConfig config;

		fb_config_wrapper(
			display_wrapper& display, //
			const utki::version_duplet&,
			const ruisapp::window_parameters& window_params
		) :
			config([&]() {
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
			}())
		{}

		fb_config_wrapper(const fb_config_wrapper&) = delete;
		fb_config_wrapper& operator=(const fb_config_wrapper&) = delete;

		fb_config_wrapper(fb_config_wrapper&&) = delete;
		fb_config_wrapper& operator=(fb_config_wrapper&&) = delete;

		// no need to free fb config
		~fb_config_wrapper() = default;
	} fb_config;
#elif defined(RUISAPP_RENDER_OPENGLES)
	using fb_config_wrapper = egl_config_wrapper;
	fb_config_wrapper fb_config;
#endif

	struct xorg_visual_info_wrapper {
		XVisualInfo* const visual_info;

		xorg_visual_info_wrapper(
			display_wrapper& display, //
			fb_config_wrapper& fb_config
		) :
			visual_info([&]() {
#ifdef RUISAPP_RENDER_OPENGL
				auto visual_info = glXGetVisualFromFBConfig(
					display.xorg_display.display, //
					fb_config.config
				);
				if (!visual_info) {
					throw std::runtime_error("glXGetVisualFromFBConfig() failed");
				}
				return visual_info;
#elif defined(RUISAPP_RENDER_OPENGLES)
				EGLint vid = 0;

				if (!eglGetConfigAttrib(
						display.egl_display.display, //
						fb_config.config,
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
#else
#	error "Unknown graphics API"
#endif
			}())
		{}

		xorg_visual_info_wrapper(const xorg_visual_info_wrapper&) = delete;
		xorg_visual_info_wrapper& operator=(const xorg_visual_info_wrapper&) = delete;

		xorg_visual_info_wrapper(xorg_visual_info_wrapper&&) = delete;
		xorg_visual_info_wrapper& operator=(xorg_visual_info_wrapper&&) = delete;

		~xorg_visual_info_wrapper()
		{
			XFree(this->visual_info);
		}
	} xorg_visual_info;

	struct xorg_color_map_wrapper {
		display_wrapper& display;

		const Colormap color_map;

		xorg_color_map_wrapper(
			display_wrapper& display, //
			xorg_visual_info_wrapper& visual_info
		) :
			display(display),
			color_map([&]() {
				auto cm = XCreateColormap(
					this->display.xorg_display.display,
					this->display.xorg_display.get_root_window(visual_info.visual_info->screen),
					visual_info.visual_info->visual,
					AllocNone
				);
				if (cm == None) {
					// TODO: use XSetErrorHandler() to get error code
					throw std::runtime_error("XCreateColormap(): failed");
				}
				return cm;
			}())
		{}

		xorg_color_map_wrapper(const xorg_color_map_wrapper&) = delete;
		xorg_color_map_wrapper& operator=(const xorg_color_map_wrapper&) = delete;

		xorg_color_map_wrapper(xorg_color_map_wrapper&&) = delete;
		xorg_color_map_wrapper& operator=(xorg_color_map_wrapper&&) = delete;

		~xorg_color_map_wrapper()
		{
			XFreeColormap(
				display.xorg_display.display, //
				this->color_map
			);
		}
	} xorg_color_map;

	struct xorg_window_wrapper {
		display_wrapper& display;

		const ::Window window;

		xorg_window_wrapper(
			display_wrapper& display, //
			const ruisapp::window_parameters& window_params,
			xorg_color_map_wrapper& color_map,
			xorg_visual_info_wrapper& visual_info,
			bool visible
		) :
			display(display),
			window([&]() {
				XSetWindowAttributes attr;
				attr.colormap = color_map.color_map;
				attr.border_pixel = 0;
				attr.background_pixmap = None;
				attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
					PointerMotionMask | ButtonMotionMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask;
				unsigned long fields = CWBorderPixel | CWColormap | CWEventMask | CWBackPixmap;

				auto dims = (this->display.scale_factor * window_params.dims.to<ruis::real>()).to<unsigned>();

				auto w = XCreateWindow(
					this->display.xorg_display.display,
					this->display.xorg_display.get_root_window(visual_info.visual_info->screen), // parent window
					0, // x position
					0, // y position
					dims.x(), // width
					dims.y(), // height
					0, // border width
					visual_info.visual_info->depth, // window's depth
					InputOutput, // window's class
					visual_info.visual_info->visual,
					fields, // defined attributes
					&attr
				);
				if (!w) {
					throw std::runtime_error("Failed to create window");
				}
				return w;
			}())
		{
			{ // we want to handle WM_DELETE_WINDOW event to know when window is closed
				Atom a = XInternAtom(
					this->display.xorg_display.display, //
					"WM_DELETE_WINDOW",
					True
				);
				XSetWMProtocols(
					this->display.xorg_display.display, //
					this->window,
					&a,
					1
				);
			}

			if (!window_params.taskbar) {
				Atom wm_state = XInternAtom(
					this->display.xorg_display.display, //
					"_NET_WM_STATE",
					False
				);
				Atom skip_taskbar = XInternAtom(
					this->display.xorg_display.display, //
					"_NET_WM_STATE_SKIP_TASKBAR",
					False
				);

				static_assert(sizeof(skip_taskbar) >= 4, "Atom must be of at least 32-bit size");
				XChangeProperty(
					this->display.xorg_display.display, //
					this->window,
					wm_state,
					XA_ATOM, // type of data
					4 * utki::byte_bits, // data is a list of 32-bit values
					PropModeReplace,
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, "using C API")
					reinterpret_cast<unsigned char*>(&skip_taskbar), // data
					1 // only one value in the data
				);
			}

			if (visible) {
				XMapWindow(
					this->display.xorg_display.display, //
					this->window
				);
			}

			this->display.xorg_display.flush();

			if (!window_params.title.empty()) {
				XStoreName(
					this->display.xorg_display.display, //
					this->window,
					window_params.title.c_str()
				);
			}
		}

		xorg_window_wrapper(const xorg_window_wrapper&) = delete;
		xorg_window_wrapper& operator=(const xorg_window_wrapper&) = delete;

		xorg_window_wrapper(xorg_window_wrapper&&) = delete;
		xorg_window_wrapper& operator=(xorg_window_wrapper&&) = delete;

		~xorg_window_wrapper()
		{
			XDestroyWindow(
				this->display.xorg_display.display, //
				this->window
			);
		}
	} xorg_window;

#ifdef RUISAPP_RENDER_OPENGL
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
			const xorg_visual_info_wrapper& visual_info,
			const utki::version_duplet& gl_version,
			const fb_config_wrapper& fb_config,
			native_window* shared_gl_context_native_window
		) :
			display(display),
			supported_extensions([&]() {
				utki::flags<glx_extension> supported = false;

				auto glx_extensions_string = std::string_view(glXQueryExtensionsString(
					this->display.xorg_display.display, //
					visual_info.visual_info->screen
				));
				utki::log_debug([&](auto& o) {
					o << "glx_extensions_string = " << glx_extensions_string << std::endl;
				});

				auto glx_extensions = utki::split(glx_extensions_string);

				using namespace std::string_view_literals;

				if (std::find(
						glx_extensions.begin(), //
						glx_extensions.end(),
						"GLX_ARB_create_context"sv
					) != glx_extensions.end())
				{
					supported.set(glx_extension::glx_arb_create_context);
				}

				if (std::find(
						glx_extensions.begin(), //
						glx_extensions.end(),
						"GLX_EXT_swap_control"sv
					) != glx_extensions.end())
				{
					supported.set(glx_extension::glx_ext_swap_control);
				}

				if (std::find( //
						glx_extensions.begin(),
						glx_extensions.end(),
						"GLX_MESA_swap_control"sv
					) != glx_extensions.end())
				{
					supported.set(glx_extension::glx_mesa_swap_control);
				}

				return supported;
			}()),
			context([&]() {
				GLXContext gl_context = nullptr;

				GLXContext shared_glx_context = [&]() -> GLXContext {
					if (shared_gl_context_native_window) {
						return shared_gl_context_native_window->glx_context.context;
					}
					return nullptr;
				}();

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
						fb_config.config,
						shared_glx_context,
						GL_TRUE,
						context_attribs.data()
					);
				} else {
					// GLX_ARB_create_context is not supported
					gl_context = glXCreateContext(
						this->display.xorg_display.display, //
						visual_info.visual_info,
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
	} glx_context;

#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_surface_wrapper egl_surface;
	egl_context_wrapper egl_context;
#endif

	struct xorg_input_context_wrapper {
		const XIC input_context;

		xorg_input_context_wrapper(
			const display_wrapper& display, //
			const xorg_window_wrapper& window
		) :
			input_context([&]() {
				auto ic =
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, "using C API")
					XCreateIC(
						display.xorg_input_method.xim,
						XNClientWindow,
						window.window,
						XNFocusWindow,
						window.window,
						XNInputStyle,
						XIMPreeditNothing | XIMStatusNothing,
						nullptr
					);
				if (ic == nullptr) {
					throw std::runtime_error("XCreateIC() failed");
				}
				return ic;
			}())
		{}

		xorg_input_context_wrapper(const xorg_input_context_wrapper&) = delete;
		xorg_input_context_wrapper& operator=(const xorg_input_context_wrapper&) = delete;

		xorg_input_context_wrapper(xorg_input_context_wrapper&&) = delete;
		xorg_input_context_wrapper& operator=(xorg_input_context_wrapper&&) = delete;

		~xorg_input_context_wrapper()
		{
			XUnsetICFocus(this->input_context);
			XDestroyIC(this->input_context);
		}
	} xorg_input_context;

public:
	native_window(
		utki::shared_ref<display_wrapper> display, //
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	) :
		display(std::move(display)),
		fb_config(
#ifdef RUISAPP_RENDER_OPENGL
			this->display,
#elif defined(RUISAPP_RENDER_OPENGLES)
			this->display.get().egl_display,
#endif
			gl_version,
			window_params
		),
		xorg_visual_info(
			this->display, //
			this->fb_config
		),
		xorg_color_map(
			this->display, //
			this->xorg_visual_info
		),
		xorg_window(
			this->display, //
			window_params,
			this->xorg_color_map,
			this->xorg_visual_info,
			shared_gl_context_native_window != nullptr
		),
#ifdef RUISAPP_RENDER_OPENGL
		glx_context(
			this->display, //
			this->xorg_visual_info,
			gl_version,
			this->fb_config,
			shared_gl_context_native_window
		),
#elif defined(RUISAPP_RENDER_OPENGLES)
		egl_surface(
			this->display.get().egl_display, //
			this->fb_config,
			this->xorg_window.window
		),
		egl_context(
			this->display.get().egl_display, //
			gl_version,
			this->fb_config,
			shared_gl_context_native_window ? shared_gl_context_native_window->egl_context.context : EGL_NO_CONTEXT
		),
#endif
		xorg_input_context(
			this->display, //
			this->xorg_window
		)
	{
#ifdef RUISAPP_RENDER_OPENGL
		// if there are no any GL contexts current, then set this one
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

	void set_vsync_enabled(bool enable) noexcept override
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
				this->xorg_window.window,
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
		if (eglSwapInterval(
				this->display.get().egl_display.display, //
				enable ? 1 : 0 // swap interval in vsync frames
			) != EGL_TRUE)
		{
			utki::logcat("WARNING: eglSwapInterval(", enable, ") failed");
		}
#endif
	}

	void swap_frame_buffers() override
	{
#ifdef RUISAPP_RENDER_OPENGL
		glXSwapBuffers(
			this->display.get().xorg_display.display, //
			this->xorg_window.window
		);
#elif defined(RUISAPP_RENDER_OPENGLES)
		this->egl_surface.swap_frame_buffers();
#else
#	error "Unknown graphics API"
#endif
	}

	static_assert(std::is_integral_v<::Window>, "xorg lib's Window type is unexpectedly not integral");
	using window_id_type = ::Window;

	window_id_type get_id() const noexcept
	{
		return this->xorg_window.window;
	}

	void set_fullscreen_internal(bool enable) override
	{
		Atom state_atom = XInternAtom(
			this->display.get().xorg_display.display, //
			"_NET_WM_STATE",
			False
		);
		Atom atom = XInternAtom(
			this->display.get().xorg_display.display, //
			"_NET_WM_STATE_FULLSCREEN",
			False
		);

		XEvent event;
		event.xclient.type = ClientMessage;
		event.xclient.serial = 0;
		event.xclient.send_event = True;
		event.xclient.window = this->xorg_window.window;
		event.xclient.message_type = state_atom;

		// data should be viewed as list of longs
		event.xclient.format = utki::byte_bits * 4;

		// use union from third-party code
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[0] = enable ? 1 : 0;

		// use union from third-party code
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[1] = long(atom);

		// use union from third-party code
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[2] = 0;

		XSendEvent(
			this->display.get().xorg_display.display, //
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			this->display.get().xorg_display.get_default_root_window(),
			False,
			SubstructureRedirectMask | SubstructureNotifyMask,
			&event
		);

		this->display.get().xorg_display.flush();
	}

	void set_mouse_cursor(ruis::mouse_cursor c) override
	{
		this->cur_cursor = &this->display.get().get_cursor(c);

		if (this->cursor_visible) {
			this->apply_cursor(*this->cur_cursor);
		}
	}

	void set_mouse_cursor_visible(bool visible) override
	{
		this->cursor_visible = visible;
		if (visible) {
			if (this->cur_cursor) {
				this->apply_cursor(*this->cur_cursor);
			} else {
				XUndefineCursor(
					this->display.get().xorg_display.display, //
					this->xorg_window.window
				);
			}
		} else {
			this->apply_cursor(this->display.get().get_cursor(ruis::mouse_cursor::none));
		}
	}

	std::u32string get_string(XKeyEvent& event) const
	{
#ifndef X_HAVE_UTF8_STRING
#	error "no Xutf8stringlookup()"
#endif
		constexpr auto static_buf_size = 32;

		// the array is used to get data, so no need to initialize it
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
		std::array<char, static_buf_size> static_buf;

		std::vector<char> arr;
		auto buf = utki::make_span(static_buf);

		// the variable is initialized via output argument, so no need to initialize
		// it here NOLINTNEXTLINE(cppcoreguidelines-init-variables)
		Status status;

		int size = Xutf8LookupString(
			this->xorg_input_context.input_context, //
			&event,
			buf.data(),
			int(buf.size() - 1),
			nullptr,
			&status
		);
		if (status == XBufferOverflow) {
			// allocate enough memory
			arr.resize(size + 1);
			buf = utki::make_span(arr);
			size = Xutf8LookupString(
				this->xorg_input_context.input_context, //
				&event,
				buf.data(),
				int(buf.size() - 1),
				nullptr,
				&status
			);
		}
		utki::assert(size >= 0, SL);
		utki::assert(buf.size() != 0, SL);
		utki::assert(buf.size() > unsigned(size), SL);

		buf[size] = 0; // null-terminate

		switch (status) {
			case XLookupChars:
			case XLookupBoth:
				if (size == 0) {
					return {};
				}
				return utki::to_utf32(buf.data());
			default:
			case XBufferOverflow:
				ASSERT(false)
			case XLookupKeySym:
			case XLookupNone:
				break;
		}

		return {};
	}

private:
	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	cursor_wrapper* cur_cursor = nullptr;
	bool cursor_visible = true;

	void apply_cursor(cursor_wrapper& c)
	{
		// set the cursor to xorg window
		XDefineCursor(
			this->display.get().xorg_display.display, //
			this->xorg_window.window,
			c.cursor
		);
	}

	void bind_rendering_context() override
	{
#ifdef RUISAPP_RENDER_OPENGL
		glXMakeCurrent(
			this->display.get().xorg_display.display, //
			this->xorg_window.window,
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
};

} // namespace
