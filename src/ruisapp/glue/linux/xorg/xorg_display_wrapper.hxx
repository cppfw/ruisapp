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

#include <stdexcept>

#include <X11/Xlib.h>

namespace {

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

	xorg_display_wrapper(const xorg_display_wrapper&) = delete;
	xorg_display_wrapper& operator=(const xorg_display_wrapper&) = delete;

	xorg_display_wrapper(xorg_display_wrapper&&) = delete;
	xorg_display_wrapper& operator=(xorg_display_wrapper&&) = delete;

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
};

} // namespace
