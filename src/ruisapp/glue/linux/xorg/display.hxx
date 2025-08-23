
#pragma once

#include <gdk/gdk.h>

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glx.h>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#endif

#include "cursor.hxx"
#include "xorg_display_wrapper.hxx"

namespace {
class display_wrapper
{
public:
	xorg_display_wrapper xorg_display;

	struct xorg_input_method_wrapper {
		const XIM xim;

		xorg_input_method_wrapper(const xorg_display_wrapper& display) :
			xim([&]() {
				auto xim = XOpenIM(
					display.display, //
					nullptr,
					nullptr,
					nullptr
				);
				if (xim == nullptr) {
					throw std::runtime_error("XOpenIM() failed");
				}
				return xim;
			}())
		{}

		~xorg_input_method_wrapper()
		{
			XCloseIM(this->xim);
		}
	} xorg_input_method;

#if defined(RUISAPP_RENDER_OPENGLES)
	struct egl_display_wrapper {
		const EGLDisplay display;

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

	const ruis::real scale_factor;

	display_wrapper() :
		xorg_input_method(this->xorg_display),
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

	ruis::real get_dots_per_inch()
	{
		int src_num = 0;

		constexpr auto mm_per_cm = 10;

		ruis::real value =
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			((ruis::real(DisplayWidth(this->xorg_display.display, src_num))
			  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			  / (ruis::real(DisplayWidthMM(this->xorg_display.display, src_num)) / ruis::real(mm_per_cm)))
			 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			 +
			 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			 (ruis::real(DisplayHeight(this->xorg_display.display, src_num))
			  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			  / (ruis::real(DisplayHeightMM(this->xorg_display.display, src_num)) / ruis::real(mm_per_cm)))) /
			2;
		value *= ruis::real(utki::cm_per_inch);
		return value;
	}

	ruis::real get_dots_per_pp()
	{
		// TODO: use scale factor only for desktop monitors
		if (auto scale_factor = this->scale_factor; scale_factor != ruis::real(1)) {
			return scale_factor;
		}

		int src_num = 0;
		r4::vector2<unsigned> resolution(
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayWidth(this->xorg_display.display, src_num),
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayHeight(this->xorg_display.display, src_num)
		);
		r4::vector2<unsigned> screen_size_mm(
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayWidthMM(this->xorg_display.display, src_num),
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayHeightMM(this->xorg_display.display, src_num)
		);

		return ruisapp::application::get_pixels_per_pp(resolution, screen_size_mm);
	}

	cursor_wrapper& get_cursor(ruis::mouse_cursor c)
	{
		auto& p = this->cursors[c];
		if (!p) {
			p = std::make_unique<cursor_wrapper>(this->xorg_display, c);
		}
		return *p;
	}

private:
	utki::enum_array<std::unique_ptr<cursor_wrapper>, ruis::mouse_cursor> cursors;
};
} // namespace
