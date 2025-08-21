
namespace {
class display_wrapper
{
	struct xorg_display_wrapper{
		Display* display;

		xorg_display_wrapper(){
			this->display = XOpenDisplay(nullptr);
			if (!this->display) {
				throw std::runtime_error("XOpenDisplay() failed");
			}
		}

		~xorg_display_wrapper(){
			XCloseDisplay(this->display);
		}

		void flush()
		{
			XFlush(this->display);
		}

		Window& get_default_root_window(){
			return DefaultRootWindow(this->display);
		}
	} xorg_display_v;

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
#endif

public:
	const ruis::real scale_factor;

	display_wrapper() :
		input_method_v(this->display()),
		scale_factor([](){
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
	{}

	display_wrapper(const xorg_display_wrapper&) = delete;
	display_wrapper& operator=(const xorg_display_wrapper&) = delete;

	display_wrapper(xorg_display_wrapper&&) = delete;
	display_wrapper& operator=(xorg_display_wrapper&&) = delete;

	Display* display()
	{
		return this->xorg_display_v.display;
	}

	XIM& input_method()
	{
		return this->input_method_v.xim;
	}

	void flush()
	{
		this->xorg_display_v.flush();
	}

    Window& get_default_root_window(){
        return this->xorg_display_v.get_default_root_window();
    }
};
} // namespace
