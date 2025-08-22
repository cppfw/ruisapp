namespace {

class native_window : public ruisapp::window
{
	utki::shared_ref<display_wrapper> display;

#ifdef RUISAPP_RENDER_OPENGL
	struct fb_config_wrapper {
		GLXFBConfig config;

		fb_config_wrapper(
			display_wrapper& display, //
			const utki::version_duplet& gl_version,
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

		~xorg_window_wrapper()
		{
			XDestroyWindow(
				this->display.xorg_display.display, //
				this->window
			);
		}
	} xorg_window;

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
		)
	{}

	native_window(const native_window&) = delete;
	native_window& operator=(const native_window&) = delete;

	native_window(native_window&&) = delete;
	native_window& operator=(native_window&&) = delete;

	// TODO: remove
#ifdef RUISAPP_RENDER_OPENGL
	GLXFBConfig& xorg_fb_config()
	{
		return this->fb_config.config;
	}
#elif defined(RUISAPP_RENDER_OPENGLES)
	EGLConfig& egl_config()
	{
		return this->fb_config.config;
	}
#endif
	XVisualInfo* visual_info()
	{
		return this->xorg_visual_info.visual_info;
	}

	Colormap color_map()
	{
		return this->xorg_color_map.color_map;
	}

	::Window win()
	{
		return this->xorg_window.window;
	}
};

} // namespace
