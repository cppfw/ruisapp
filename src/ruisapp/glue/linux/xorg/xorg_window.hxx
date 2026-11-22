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

// A base X11 native window that implements everything that is not related to
// the graphics API: X11 window creation, keyboard input (via XIM/IME), mouse
// cursor handling, fullscreen, cascading, etc.
//
// The graphics-specific parts (visual selection, graphics context creation,
// swap_frame_buffers, bind_rendering_context) are implemented by the derived
// classes (see window.hxx for OpenGL/OpenGL ES and vulkan_window.hxx for
// Vulkan).

#pragma once

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <ruis/render/native_window.hpp>

#include "display.hxx"

namespace {

// new windows are cascaded to prevent overlap
constexpr unsigned cascade_max_columns = 15;
constexpr int cascade_step_px = 60;
constexpr int cascade_hint_step_px = 40;

// the cascade offset (in pixels) for the window at the given index
inline int cascade_offset(unsigned index, int step_px)
{
	return int(index % cascade_max_columns) * step_px;
}

// the X11 visual info needed to create a window. The graphics API determines
// which visual to use (OpenGL/OpenGL ES pick an FBConfig-based visual, Vulkan
// uses the default visual)
struct xorg_visual {
	Visual* visual;
	unsigned depth;
};

class xorg_window : public ruis::render::native_window
{
	struct xorg_color_map_wrapper {
		display_wrapper& display;

		const Colormap color_map;

		xorg_color_map_wrapper(
			display_wrapper& display, //
			Visual* visual
		) :
			display(display),
			color_map([&]() {
				auto cm = XCreateColormap(
					this->display.xorg_display.display,
					this->display.xorg_display.get_root_window(this->display.xorg_display.get_default_screen()),
					visual,
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
		xorg_color_map_wrapper& operator=(const xorg_color_map_wrapper&&) = delete;

		~xorg_color_map_wrapper()
		{
			XFreeColormap(
				this->display.xorg_display.display, //
				this->color_map
			);
		}
	};

	struct xorg_window_wrapper {
		display_wrapper& display;

		const ::Window window;
		mutable bool destroyed = false;

		xorg_window_wrapper(
			display_wrapper& display, //
			const ruisapp::window_parameters& window_params,
			const xorg_color_map_wrapper& color_map,
			Visual* visual,
			unsigned depth,
			bool visible,
			int cascade_px,
			int hint_px
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
					this->display.xorg_display.get_root_window(this->display.xorg_display.get_default_screen()
					), // parent
					cascade_px, // x position
					cascade_px, // y position
					dims.x(), // width
					dims.y(), // height
					0, // border width
					int(depth), // window's depth (bits per pixel)
					InputOutput, // window's class
					visual,
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

			{
				// set the window position hint so the WM respects the cascading position
				auto* hints = XAllocSizeHints();
				hints->flags = USPosition;
				hints->x = hint_px;
				hints->y = hint_px;
				XSetWMNormalHints(this->display.xorg_display.display, this->window, hints);
				XFree(hints);
			}

			if (!window_params.taskbar) {
				Atom wm_state = XInternAtom(
					this->display.xorg_display.display, //
					"_NET_WM_STATE",
					False
				);
				Atom skip_taskbar =
					XInternAtom(this->display.xorg_display.display, "_NET_WM_STATE_SKIP_TASKBAR", False);

				static_assert(sizeof(skip_taskbar) >= 4, "Atom must be of at least 32-bit size");
				XChangeProperty(
					this->display.xorg_display.display, //
					this->window,
					wm_state,
					XA_ATOM, // type of data
					4 * utki::byte_bits, // data is a list of 32-bit values
					PropModeReplace,
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, "using C API")
					reinterpret_cast<unsigned char*>(&skip_taskbar), // data
					1 // only one value in the data
				);
			}

			if (visible) {
				XMapWindow(
					this->display.xorg_display.display, //
					this->window
				);
			}

			this->display.xorg_display.flush();

			if (!window_params.title.empty()) {
				XStoreName(
					this->display.xorg_display.display, //
					this->window,
					window_params.title.c_str()
				);
			}

			// force the cascading position (WM may ignore position hints)
			XMoveWindow(
				this->display.xorg_display.display, //
				this->window,
				cascade_px,
				cascade_px
			);
		}

		xorg_window_wrapper(const xorg_window_wrapper&) = delete;
		xorg_window_wrapper& operator=(const xorg_window_wrapper&) = delete;

		xorg_window_wrapper(xorg_window_wrapper&&) = delete;
		xorg_window_wrapper& operator=(const xorg_window_wrapper&&) = delete;

		~xorg_window_wrapper()
		{
			this->destroy();
		}

		void destroy() const
		{
			if (!this->destroyed) {
				XDestroyWindow(
					this->display.xorg_display.display, //
					this->window
				);
				XFlush(this->display.xorg_display.display);
				this->destroyed = true;
			}
		}
	};

	struct xorg_input_context_wrapper {
		const XIC input_context;

		xorg_input_context_wrapper(
			const display_wrapper& display, //
			const xorg_window_wrapper& window
		) :
			input_context([&]() {
				auto ic =
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, "using C API")
					XCreateIC(
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
		xorg_input_context_wrapper& operator=(const xorg_input_context_wrapper&&) = delete;

		~xorg_input_context_wrapper()
		{
			XUnsetICFocus(this->input_context);
			XDestroyIC(this->input_context);
		}
	};

public:
	// counter for cascading new windows to prevent overlap
	static inline unsigned s_window_count = 0;

	xorg_window(
		utki::shared_ref<display_wrapper> display, //
		const ruisapp::window_parameters& window_params,
		const xorg_window* shared,
		const xorg_visual& visual
	) :
		display(std::move(display)),
		colormap(
			this->display, //
			visual.visual
		),
		window(
			this->display, //
			window_params,
			this->colormap,
			visual.visual,
			visual.depth,
			shared != nullptr, // visible
			cascade_offset(s_window_count, cascade_step_px),
			cascade_offset(s_window_count, cascade_hint_step_px)
		),
		input_context(
			this->display, //
			this->window
		),
		current_dims((this->display.get().scale_factor * window_params.dims.to<ruis::real>()).to<unsigned>())
	{
		s_window_count++;
	}

	xorg_window(const xorg_window&) = delete;
	xorg_window& operator=(const xorg_window&) = delete;

	xorg_window(xorg_window&&) = delete;
	xorg_window& operator=(const xorg_window&&) = delete;

	~xorg_window() override = default;

	static_assert(std::is_integral_v<::Window>, "xorg lib's Window type is unexpectedly not integral");
	using window_id_type = ::Window;

	window_id_type get_id() const noexcept
	{
		return this->window.window;
	}

	r4::vector2<unsigned> get_dims() const noexcept override
	{
		return this->current_dims;
	}

	void set_dims(const r4::vector2<unsigned> dims) const noexcept override
	{
		XResizeWindow(this->display.get().xorg_display.display, this->window.window, dims.x(), dims.y());
		this->current_dims = dims;
	}

	void set_fullscreen_internal(bool enable) override
	{
		Atom state_atom = XInternAtom(
			this->display.get().xorg_display.display, //
			"_NET_WM_STATE",
			False
		);
		Atom atom = XInternAtom(
			this->display.get().xorg_display.display, //
			"_NET_WM_STATE_FULLSCREEN",
			False
		);

		XEvent event;
		event.xclient.type = ClientMessage;
		event.xclient.serial = 0;
		event.xclient.send_event = True;
		event.xclient.window = this->window.window;
		event.xclient.message_type = state_atom;

		// data should be viewed as list of longs
		event.xclient.format = utki::byte_bits * 4;

		// use union from third-party code
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[0] = enable ? 1 : 0;

		// use union from third-party code
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[1] = long(atom);

		// use union from third-party code
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[2] = 0;

		XSendEvent(
			this->display.get().xorg_display.display, //
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			this->display.get().xorg_display.get_default_root_window(),
			False,
			SubstructureRedirectMask | SubstructureNotifyMask,
			&event
		);

		this->display.get().xorg_display.flush();
	}

	void set_mouse_cursor(ruis::mouse_cursor c) override
	{
		this->cur_cursor = &this->display.get().get_cursor(c);

		if (this->cursor_visible) {
			this->apply_cursor(*this->cur_cursor);
		}
	}

	void set_mouse_cursor_visible(bool visible) override
	{
		this->cursor_visible = visible;
		if (visible) {
			if (this->cur_cursor) {
				this->apply_cursor(*this->cur_cursor);
			} else {
				XUndefineCursor(
					this->display.get().xorg_display.display, //
					this->window.window
				);
			}
		} else {
			this->apply_cursor(this->display.get().get_cursor(ruis::mouse_cursor::none));
		}
	}

	std::u32string get_string(XKeyEvent& event) const
	{
#ifndef X_HAVE_UTF8_STRING
#	error "no Xutf8stringlookup()"
#endif
		constexpr auto static_buf_size = 32;

		// the array is used to get data, so no need to initialize it
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
		std::array<char, static_buf_size> static_buf;

		std::vector<char> arr;
		auto buf = utki::make_span(static_buf);

		// the variable is initialized via output argument, so no need to initialize
		// it here NOLINTNEXTLINE(cppcoreguidelines-init-variables)
		Status status;

		int size = Xutf8LookupString(
			this->input_context.input_context, //
			&event,
			buf.data(),
			int(buf.size() - 1),
			nullptr,
			&status
		);
		if (status == XBufferOverflow) {
			// allocate enough memory
			arr.resize(size + 1);
			buf = utki::make_span(arr);
			size = Xutf8LookupString(
				this->input_context.input_context, //
				&event,
				buf.data(),
				int(buf.size() - 1),
				nullptr,
				&status
			);
		}
		utki::assert(size >= 0, SL);
		utki::assert(buf.size() != 0, SL);
		utki::assert(buf.size() > unsigned(size), SL);

		buf[size] = 0; // null-terminate

		switch (status) {
			case XLookupChars:
			case XLookupBoth:
				if (size == 0) {
					return {};
				}
				return utki::to_utf32(buf.data());
			default:
			case XBufferOverflow:
				utki::assert(false, SL);
			case XLookupKeySym:
			case XLookupNone:
				break;
		}

		return {};
	}

protected:
	const utki::shared_ref<display_wrapper> display;
	const xorg_color_map_wrapper colormap;
	const xorg_window_wrapper window;
	const xorg_input_context_wrapper input_context;
	mutable r4::vector2<unsigned> current_dims;

private:
	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	cursor_wrapper* cur_cursor = nullptr;
	bool cursor_visible = true;

	void apply_cursor(cursor_wrapper& c)
	{
		// set the cursor to xorg window
		XDefineCursor(
			this->display.get().xorg_display.display, //
			this->window.window,
			c.cursor
		);
	}
};

} // namespace
