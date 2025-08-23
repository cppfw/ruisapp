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
