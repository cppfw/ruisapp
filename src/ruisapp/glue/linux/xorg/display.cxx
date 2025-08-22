
namespace {
class display_wrapper
{
public:
	struct xorg_display_wrapper {
		Display* const display;

		xorg_display_wrapper() :
			display([]() {
				auto d = XOpenDisplay(nullptr);
				if (!d) {
					throw std::runtime_error("XOpenDisplay() failed");
				}
				return d;
			}())
		{}

		~xorg_display_wrapper()
		{
			XCloseDisplay(this->display);
		}

		void flush()
		{
			XFlush(this->display);
		}

		Window& get_default_root_window()
		{
			return DefaultRootWindow(this->display);
		}

		Window& get_root_window(int screen)
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)

			return RootWindow(
				this->display, //
				screen
			);
		}

		int get_default_screen()
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, "the cast is inside of xlib macro")
			return DefaultScreen(this->display);
		}
	} xorg_display;

private:
	struct xorg_input_method_wrapper {
		XIM xim;

		xorg_input_method_wrapper(Display* display)
		{
			this->xim = XOpenIM(
				display, //
				nullptr,
				nullptr,
				nullptr
			);
			if (this->xim == nullptr) {
				throw std::runtime_error("XOpenIM() failed");
			}
		}

		~xorg_input_method_wrapper()
		{
			XCloseIM(this->xim);
		}
	} input_method_v;

#if defined(RUISAPP_RENDER_OPENGLES)

public:
	struct egl_display_wrapper {
		EGLDisplay display;

		egl_display_wrapper() :
			display([]() {
				auto d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
				if (d == EGL_NO_DISPLAY) {
					throw std::runtime_error("eglGetDisplay(): failed, no matching display connection found");
				}
				return d;
			}())
		{
			try {
				if (eglInitialize(
						this->display, //
						nullptr,
						nullptr
					) == EGL_FALSE)
				{
					throw std::runtime_error("eglInitialize() failed");
				}

				if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
					throw std::runtime_error("eglBindApi() failed");
				}
			} catch (...) {
				eglTerminate(this->display);
				throw;
			}
		}

		~egl_display_wrapper()
		{
			eglTerminate(this->display);
		}
	} egl_display;
#endif

public:
	const ruis::real scale_factor;

	display_wrapper() :
		input_method_v(this->xorg_display.display),
		scale_factor([]() {
			// get scale factor
			gdk_init(nullptr, nullptr);

			// GDK-4 version commented out because GDK-4 is not available in Debian 11

			// auto display_name = DisplayString(ww.display.display);
			// std::cout << "display name = " << display_name << std::endl;
			// auto disp = gdk_display_open(display_name);
			// utki::assert(disp, SL);
			// std::cout << "gdk display name = " << gdk_display_get_name(disp) << std::endl;
			// auto surf = gdk_surface_new_toplevel (disp);
			// utki::assert(surf, SL);
			// auto mon = gdk_display_get_monitor_at_surface (disp, surf);
			// utki::assert(mon, SL);
			// int sf = gdk_monitor_get_scale_factor(mon);

			// GDK-3 version
			int sf = gdk_window_get_scale_factor(gdk_get_default_root_window());
			auto scale_factor = ruis::real(sf);

			std::cout << "display scale factor = " << scale_factor << std::endl;
			return scale_factor;
		}())
	{
#if defined(RUISAPP_RENDER_OPENGL)
		{
			int glx_ver_major = 0;
			int glx_ver_minor = 0;
			if (!glXQueryVersion(
					this->xorg_display.display, //
					&glx_ver_major,
					&glx_ver_minor
				))
			{
				throw std::runtime_error("glXQueryVersion() failed");
			}

			// we need the following:
			// - glXQueryExtensionsString(), availabale starting from GLX version 1.1
			// - FBConfigs, availabale starting from GLX version 1.3
			// minimum GLX version needed is 1.3
			if (glx_ver_major < 1 || (glx_ver_major == 1 && glx_ver_minor < 3)) {
				throw std::runtime_error("GLX version 1.3 or above is required");
			}
		}
#endif
	}

	display_wrapper(const xorg_display_wrapper&) = delete;
	display_wrapper& operator=(const xorg_display_wrapper&) = delete;

	display_wrapper(xorg_display_wrapper&&) = delete;
	display_wrapper& operator=(xorg_display_wrapper&&) = delete;

	XIM& input_method()
	{
		return this->input_method_v.xim;
	}
};
} // namespace
