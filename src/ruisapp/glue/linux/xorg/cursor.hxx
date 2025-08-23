#pragma once

#include <X11/cursorfont.h>
#include <utki/enum_array.hpp>

#include "xorg_display_wrapper.hxx"

namespace {
struct cursor_wrapper {
	friend class os_platform_glue;

	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	xorg_display_wrapper& xorg_display;
	const Cursor cursor; // xorg cursor

	constexpr static const utki::enum_array<unsigned, ruis::mouse_cursor> ruis_to_x_cursor_map = {
		XC_left_ptr, // ruis::mouse_cursor::none
		XC_left_ptr, // ruis::mouse_cursor::arrow
		XC_sb_h_double_arrow, // ruis::mouse_cursor::left_right_arrow
		XC_sb_v_double_arrow, // ruis::mouse_cursor::up_down_arrow
		XC_fleur, // ruis::mouse_cursor::all_directions_arrow
		XC_left_side, // ruis::mouse_cursor::left_side
		XC_right_side, // ruis::mouse_cursor::right_side
		XC_top_side, // ruis::mouse_cursor::top_side
		XC_bottom_side, // ruis::mouse_cursor::bottom_side
		XC_top_left_corner, // ruis::mouse_cursor::top_left_corner
		XC_top_right_corner, // ruis::mouse_cursor::top_right_corner
		XC_bottom_left_corner, // ruis::mouse_cursor::bottom_left_corner
		XC_bottom_right_corner, // ruis::mouse_cursor::bottom_right_corner
		XC_hand2, // ruis::mouse_cursor::index_finger
		XC_hand1, // ruis::mouse_cursor::grab
		XC_xterm // ruis::mouse_cursor::caret
	};

	cursor_wrapper(
		xorg_display_wrapper& display, //
		ruis::mouse_cursor c
	) :
		xorg_display(display),
		cursor([&]() {
			if (c == ruis::mouse_cursor::none) {
				std::array<char, 1> data = {0};

				Pixmap blank = XCreateBitmapFromData(
					this->xorg_display.display, //
					this->xorg_display.get_default_root_window(), // only used to deremine screen and depth
					data.data(),
					1,
					1
				);
				if (blank == None) {
					throw std::runtime_error(
						"application::XEmptyMouseCursor::XEmptyMouseCursor(): could not "
						"create bitmap"
					);
				}
				utki::scope_exit scope_exit([this, &blank]() {
					XFreePixmap(
						this->xorg_display.display, //
						blank
					);
				});

				XColor dummy;
				auto c = XCreatePixmapCursor(
					this->xorg_display.display, //
					blank,
					blank,
					&dummy,
					&dummy,
					0,
					0
				);
				if (c == None) {
					throw std::runtime_error("XCreatePixmapCursor() failed");
				}
				return c;
			} else {
				auto cur = XCreateFontCursor(
					this->xorg_display.display, //
					ruis_to_x_cursor_map[c]
				);
				if (cur == None) {
					throw std::runtime_error("XCreatePixmapCursor() failed");
				}
				return cur;
			}
		}())
	{}

	cursor_wrapper(const cursor_wrapper&) = delete;
	cursor_wrapper& operator=(const cursor_wrapper&) = delete;

	cursor_wrapper(cursor_wrapper&&) = delete;
	cursor_wrapper& operator=(cursor_wrapper&&) = delete;

	~cursor_wrapper()
	{
		XFreeCursor(
			this->xorg_display.display, //
			this->cursor
		);
	}
};
} // namespace
