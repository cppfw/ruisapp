// TODO: move to glue.cxx
class ruisapp::application::platform_glue : public utki::destructable
{
public:
	nitki::queue ui_queue;

	std::atomic_bool quit_flag = false;

	ruis::real scale_factor = 1;

	platform_glue(){
		// get scale factor
		{
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
			this->scale_factor = ruis::real(sf);

			std::cout << "display scale factor = " << this->scale_factor << std::endl;
		}
	}
};
