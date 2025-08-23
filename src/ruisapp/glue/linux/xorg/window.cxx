namespace {

class native_window : public ruisapp::window
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
	struct fb_config_wrapper {
		EGLConfig config;

		fb_config_wrapper(
			display_wrapper& display,
			const utki::version_duplet& gl_version,
			const ruisapp::window_parameters& window_params
		) :
			config([&]() {
				EGLConfig egl_config = nullptr;

				// Here specify the attributes of the desired configuration.
				// Below, we select an EGLConfig with at least 8 bits per color
				// component compatible with on-screen windows.
				const std::array<EGLint, 15> attribs = {
					EGL_SURFACE_TYPE,
					EGL_WINDOW_BIT,
					EGL_RENDERABLE_TYPE,
					// We cannot set bits for all OpenGL ES versions because on platforms which do not
					// support later versions the matching config will not be found by eglChooseConfig().
					// So, set bits according to requested OpenGL ES version.
					[&ver = gl_version]() {
						EGLint ret = EGL_OPENGL_ES2_BIT; // OpenGL ES 2 is the minimum
						if (ver.major >= 3) {
							ret |= EGL_OPENGL_ES3_BIT;
						}
						return ret;
					}(),
					EGL_BLUE_SIZE,
					8,
					EGL_GREEN_SIZE,
					8,
					EGL_RED_SIZE,
					8,
					EGL_DEPTH_SIZE,
					window_params.buffers.get(ruisapp::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
					EGL_STENCIL_SIZE,
					window_params.buffers.get(ruisapp::buffer::stencil) ? utki::byte_bits : 0,
					EGL_NONE
				};

				// Here, the application chooses the configuration it desires. In this
				// sample, we have a very simplified selection process, where we pick
				// the first EGLConfig that matches our criteria.
				EGLint num_configs = 0;
				eglChooseConfig(
					display.egl_display.display, //
					attribs.data(),
					&egl_config,
					1,
					&num_configs
				);
				if (num_configs <= 0) {
					throw std::runtime_error("eglChooseConfig() failed, no matching config found");
				}

				utki::assert(egl_config, SL);

				return egl_config;
			}())
		{}

		fb_config_wrapper(const fb_config_wrapper&) = delete;
		fb_config_wrapper& operator=(const fb_config_wrapper&) = delete;

		fb_config_wrapper(fb_config_wrapper&&) = delete;
		fb_config_wrapper& operator=(fb_config_wrapper&&) = delete;

		// no need to free the egl config
		~fb_config_wrapper() = default;
	} fb_config;
#endif

	struct xorg_visual_info_wrapper {
		XVisualInfo* const visual_info;

		xorg_visual_info_wrapper(display_wrapper& display, fb_config_wrapper& fb_config) :
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
			xorg_visual_info_wrapper& visual_info
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

			XMapWindow(
				this->display.xorg_display.display, //
				this->window
			);

			this->display.xorg_display.flush();

			// set window title
			XStoreName(
				this->display.xorg_display.display, //
				this->window,
				window_params.title.c_str()
			);
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
			const fb_config_wrapper& fb_config
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

				if (std::find(
						glx_extensions.begin(), //
						glx_extensions.end(),
						"GLX_ARB_create_context"s
					) != glx_extensions.end())
				{
					supported.set(glx_extension::glx_arb_create_context);
				}

				if (std::find(
						glx_extensions.begin(), //
						glx_extensions.end(),
						"GLX_EXT_swap_control"s
					) != glx_extensions.end())
				{
					supported.set(glx_extension::glx_ext_swap_control);
				}

				if (std::find( //
						glx_extensions.begin(),
						glx_extensions.end(),
						"GLX_MESA_swap_control"s
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
						fb_config.config,
						nullptr,
						GL_TRUE,
						context_attribs.data()
					);
				} else {
					// GLX_ARB_create_context is not supported
					gl_context = glXCreateContext(
						this->display.xorg_display.display, //
						visual_info.visual_info,
						nullptr,
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
			glXMakeCurrent(
				this->display.xorg_display.display, //
				None,
				nullptr
			);
			glXDestroyContext(
				this->display.xorg_display.display, //
				this->context
			);
		}
	} glx_context;

#elif defined(RUISAPP_RENDER_OPENGLES)
	struct egl_surface_wrapper {
		display_wrapper& display;

		const EGLSurface surface;

		egl_surface_wrapper(
			display_wrapper& display, //
			const fb_config_wrapper& fb_config,
			const xorg_window_wrapper& xorg_window
		) :
			display(display),
			surface([&]() {
				auto s = eglCreateWindowSurface(
					this->display.egl_display.display, //
					fb_config.config,
					xorg_window.window,
					nullptr
				);
				if (s == EGL_NO_SURFACE) {
					throw std::runtime_error("eglCreateWindowSurface() failed");
				}
				return s;
			}())
		{}

		egl_surface_wrapper(const egl_surface_wrapper&) = delete;
		egl_surface_wrapper& operator=(const egl_surface_wrapper&) = delete;

		egl_surface_wrapper(egl_surface_wrapper&&) = delete;
		egl_surface_wrapper& operator=(egl_surface_wrapper&&) = delete;

		~egl_surface_wrapper()
		{
			eglDestroySurface(
				this->display.egl_display.display, //
				this->surface
			);
		}
	} egl_surface;

	struct egl_context_wrapper {
		display_wrapper& display;

		const EGLContext context;

		egl_context_wrapper(
			display_wrapper& display, //
			const utki::version_duplet& gl_version,
			const fb_config_wrapper& fb_config
		) :
			display(display),
			context([&]() {
				auto graphics_api_version = [&ver = gl_version]() {
					if (ver.to_uint32_t() == 0) {
						// default OpenGL ES version is 2.0
						return utki::version_duplet{
							.major = 2, //
							.minor = 0
						};
					}

					if (ver.major < 2) {
						throw std::invalid_argument(
							utki::cat("minimal supported OpenGL ES version is 2.0, requested: ", ver)
						);
					}
					return ver;
				}();

				constexpr auto attrs_array_size = 5;
				std::array<EGLint, attrs_array_size> context_attrs = {
					EGL_CONTEXT_MAJOR_VERSION,
					graphics_api_version.major,
					EGL_CONTEXT_MINOR_VERSION,
					graphics_api_version.minor,
					EGL_NONE
				};

				auto egl_context = eglCreateContext(
					this->display.egl_display.display, //
					fb_config.config,
					EGL_NO_CONTEXT,
					context_attrs.data()
				);
				if (egl_context == EGL_NO_CONTEXT) {
					throw std::runtime_error("eglCreateContext() failed");
				}
				return egl_context;
			}())
		{}

		egl_context_wrapper(const egl_context_wrapper&) = delete;
		egl_context_wrapper& operator=(const egl_context_wrapper&) = delete;

		egl_context_wrapper(egl_context_wrapper&&) = delete;
		egl_context_wrapper& operator=(egl_context_wrapper&&) = delete;

		~egl_context_wrapper()
		{
			eglMakeCurrent(
				this->display.egl_display.display, //
				EGL_NO_SURFACE,
				EGL_NO_SURFACE,
				EGL_NO_CONTEXT
			);
			eglDestroyContext(
				this->display.egl_display.display, //
				this->context
			);
		}
	} egl_context;
#endif

	struct xorg_input_context_wrapper {
		const XIC input_context;

		xorg_input_context_wrapper(
			const display_wrapper& display, //
			const xorg_window_wrapper& window
		) :
			input_context([&]() {
				auto ic = XCreateIC(
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

	ruis::real scale_factor = 1;

public:
	native_window(
		utki::shared_ref<display_wrapper> display, //
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params
	) :
		display(std::move(display)),
		fb_config(
			this->display, //
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
			this->xorg_visual_info
		),
#ifdef RUISAPP_RENDER_OPENGL
		glx_context(
			this->display, //
			this->xorg_visual_info,
			gl_version,
			this->fb_config
		),
#elif defined(RUISAPP_RENDER_OPENGLES)
		egl_surface(
			this->display, //
			this->fb_config,
			this->xorg_window
		),
		egl_context(
			this->display, //
			gl_version,
			this->fb_config
		),
#endif
		xorg_input_context(
			this->display, //
			this->xorg_window
		)
	{
		this->make_gl_context_current();
		this->disable_vsync();
	}

	native_window(const native_window&) = delete;
	native_window& operator=(const native_window&) = delete;

	native_window(native_window&&) = delete;
	native_window& operator=(native_window&&) = delete;

	void make_gl_context_current()
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
	};

	void disable_vsync()
	{
		this->make_gl_context_current();
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

			ASSERT(glx_swap_interval_ext)

			// disable v-sync
			glx_swap_interval_ext(
				this->display.get().xorg_display.display, //
				this->xorg_window.window,
				0
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

			ASSERT(glx_swap_interval_mesa)

			// disable v-sync
			if (glx_swap_interval_mesa(0) != 0) {
				throw std::runtime_error("glXSwapIntervalMESA() failed");
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
				0
			) != EGL_TRUE)
		{
			throw std::runtime_error("eglSwapInterval() failed");
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
		eglSwapBuffers(
			this->display.get().egl_display.display, //
			this->egl_surface.surface
		);
#else
#	error "Unknown graphics API"
#endif
	}



	// TODO: remove

	XVisualInfo* visual_info()
	{
		return this->xorg_visual_info.visual_info;
	}

	const Colormap& color_map()
	{
		return this->xorg_color_map.color_map;
	}

	::Window win()
	{
		return this->xorg_window.window;
	}

	const XIC& xic()
	{
		return this->xorg_input_context.input_context;
	}
};

} // namespace
