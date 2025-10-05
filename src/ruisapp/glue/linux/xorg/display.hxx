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

#include <gdk/gdk.h>

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glx.h>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include "../../egl_utils.hxx"
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
	egl_display_wrapper egl_display;
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
