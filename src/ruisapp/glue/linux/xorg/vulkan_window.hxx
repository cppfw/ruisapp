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

// A minimal X11 native window for the Vulkan renderer.
// Unlike the OpenGL window (window.hxx) this one does not create any GL context,
// it only creates a plain X11 window. The Vulkan surface is created by the glue
// from the X11 window and the shared Vulkan instance.

#include <functional>
#include <stdexcept>
#include <string>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <ruis/render/native_window.hpp>

#include "cursor.hxx"
#include "xorg_display_wrapper.hxx"

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

class native_window : public ruis::render::native_window
{
public:
	// a callback that presents the frame; set by the glue to call the Vulkan
	// context's present()
	std::function<void()> present_func;

	// counter for cascading new windows to prevent overlap
	static inline unsigned s_window_count = 0;

	native_window(
		const utki::shared_ref<display_wrapper>& display,
		const utki::version_duplet&, // the GL version is not used by Vulkan
		const ruisapp::window_parameters& window_params,
		const native_window* // the shared GL context is not used by Vulkan
	) :
		display(display),
		window([&]() {
			auto screen = this->display.get().xorg_display.get_default_screen();
			auto visual = DefaultVisual(this->display.get().xorg_display.display, screen);
			auto depth = DefaultDepth(this->display.get().xorg_display.display, screen);

			XSetWindowAttributes attr;
			attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
				PointerMotionMask | ButtonMotionMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask;

			auto dims = (this->display.get().scale_factor * window_params.dims.to<ruis::real>()).to<unsigned>();

			// cascade new windows to prevent overlap
			int cascade = cascade_offset(s_window_count, cascade_step_px);
			s_window_count++;

			auto w = XCreateWindow(
				this->display.get().xorg_display.display,
				this->display.get().xorg_display.get_root_window(screen),
				cascade, // x
				cascade, // y
				dims.x(),
				dims.y(),
				0, // border width
				depth,
				InputOutput,
				visual,
				CWEventMask,
				&attr
			);
			if (!w) {
				throw std::runtime_error("Failed to create X11 window");
			}
			this->current_dims = dims;
			return w;
		}())
	{
		{ // handle WM_DELETE_WINDOW to know when the window is closed
			Atom a = XInternAtom(this->display.get().xorg_display.display, "WM_DELETE_WINDOW", True);
			XSetWMProtocols(this->display.get().xorg_display.display, this->window, &a, 1);
		}

		{
			// set the window position hint so the WM respects the cascading position
			auto* hints = XAllocSizeHints();
			hints->flags = USPosition;
			hints->x = cascade_offset(s_window_count - 1, cascade_hint_step_px);
			hints->y = cascade_offset(s_window_count - 1, cascade_hint_step_px);
			XSetWMNormalHints(this->display.get().xorg_display.display, this->window, hints);
			XFree(hints);
		}

		if (!window_params.taskbar) {
			Atom wm_state = XInternAtom(this->display.get().xorg_display.display, "_NET_WM_STATE", False);
			Atom skip_taskbar =
				XInternAtom(this->display.get().xorg_display.display, "_NET_WM_STATE_SKIP_TASKBAR", False);
			static_assert(sizeof(skip_taskbar) >= 4, "Atom must be of at least 32-bit size");
			XChangeProperty(
				this->display.get().xorg_display.display,
				this->window,
				wm_state,
				XA_ATOM,
				4 * utki::byte_bits,
				PropModeReplace,
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, "using C API")
				reinterpret_cast<unsigned char*>(&skip_taskbar),
				1
			);
		}

		XMapWindow(this->display.get().xorg_display.display, this->window);

		// force the cascading position (WM may ignore position hints)
		{
			const int cascade = cascade_offset(s_window_count - 1, cascade_step_px);
			XMoveWindow(this->display.get().xorg_display.display, this->window, cascade, cascade);
		}

		this->display.get().xorg_display.flush();

		if (!window_params.title.empty()) {
			XStoreName(this->display.get().xorg_display.display, this->window, window_params.title.c_str());
		}
	}

	native_window(const native_window&) = delete;
	native_window& operator=(const native_window&) = delete;

	native_window(native_window&&) = delete;
	native_window& operator=(native_window&&) = delete;

	~native_window() override
	{
		this->destroy_x11_window();
	}

	void destroy_x11_window()
	{
		if (!this->x11_window_destroyed) {
			XDestroyWindow(this->display.get().xorg_display.display, this->window);
			XFlush(this->display.get().xorg_display.display);
			this->x11_window_destroyed = true;
		}
	}

	void swap_frame_buffers() override
	{
		if (this->present_func) {
			this->present_func();
		}
	}

	r4::vector2<unsigned> get_dims() const noexcept override
	{
		return this->current_dims;
	}

	void set_dims(const r4::vector2<unsigned> dims) const noexcept override
	{
		XResizeWindow(this->display.get().xorg_display.display, this->window, dims.x(), dims.y());
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
		const_cast<native_window*>(this)->current_dims = dims;
	}

	// the X11 window id
	static_assert(std::is_integral_v<::Window>, "xorg lib's Window type is unexpectedly not integral");
	using window_id_type = ::Window;

	window_id_type get_id() const noexcept
	{
		return this->window;
	}

	std::u32string get_string(XKeyEvent& event) const
	{
		constexpr auto static_buf_size = 32;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
		std::array<char, static_buf_size> static_buf;
		std::vector<char> arr;
		auto buf = utki::make_span(static_buf);
		XComposeStatus status;
		int size = XLookupString(&event, buf.data(), int(buf.size() - 1), nullptr, &status);
		std::vector<unsigned> ret;
		for (int i = 0; i < size; ++i) {
			ret.push_back(uint32_t(buf[i]));
		}
		return {ret.begin(), ret.end()};
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
		event.xclient.window = this->window;
		event.xclient.message_type = state_atom;
		event.xclient.format = utki::byte_bits * 4;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[0] = enable ? 1 : 0;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[1] = long(atom);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
		event.xclient.data.l[2] = 0;

		XSendEvent(
			this->display.get().xorg_display.display, //
			this->display.get().xorg_display.get_root_window(this->display.get().xorg_display.get_default_screen()), //
			False, // propagate
			SubstructureRedirectMask | SubstructureNotifyMask,
			&event
		);
		this->display.get().xorg_display.flush();
	}

	void set_mouse_cursor_visible(bool visible) override
	{
		this->cursor_visible = visible;
		auto& c = this->display.get().get_cursor(visible ? ruis::mouse_cursor::arrow : ruis::mouse_cursor::none);
		XDefineCursor(
			this->display.get().xorg_display.display, //
			this->window,
			c.cursor
		);
	}

private:
	const utki::shared_ref<display_wrapper> display;
	mutable r4::vector2<unsigned> current_dims = {0, 0};
	bool x11_window_destroyed = false;
	const ::Window window;
	bool cursor_visible = true;
};
} // namespace
