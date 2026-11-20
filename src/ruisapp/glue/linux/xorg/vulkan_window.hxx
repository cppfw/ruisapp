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

// The Vulkan native window. Inherits from xorg_window (the graphics-agnostic
// base) and adds the Vulkan-specific parts: the present callback. Unlike the
// OpenGL window it uses the default X11 visual (not an FBConfig-based one) and
// does not create any GL context. The Vulkan surface is created by the glue
// from the X11 window and the shared Vulkan instance.

#include <functional>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xorg_window.hxx"

namespace {

class native_window : public xorg_window
{
public:
	// a callback that presents the frame; set by the glue to call the Vulkan
	// context's present()
	std::function<void()> present_func;

	native_window(
		utki::shared_ref<display_wrapper> display, //
		const utki::version_duplet&, // the GL version is not used by Vulkan
		const ruisapp::window_parameters& window_params,
		const native_window* shared
	) :
		xorg_window(
			display, //
			window_params,
			shared,
			xorg_visual{
				.visual = DefaultVisual(
					display.get().xorg_display.display, //
					display.get().xorg_display.get_default_screen()
				),
				.depth = DefaultDepth(
					display.get().xorg_display.display, //
					display.get().xorg_display.get_default_screen()
				)
			}
		)
	{}

	native_window(const native_window&) = delete;
	native_window& operator=(const native_window&) = delete;

	native_window(native_window&&) = delete;
	native_window& operator=(native_window&&) = delete;

	~native_window() override = default;

	void swap_frame_buffers() override
	{
		if (this->present_func) {
			this->present_func();
		}
	}
};

} // namespace
