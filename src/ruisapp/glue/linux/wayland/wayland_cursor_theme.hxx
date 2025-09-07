#pragma once

#include <ruis/util/mouse_cursor.hpp>
#include <utki/enum_array.hpp>
#include <wayland-cursor.h>

#include "wayland_shm.hxx"

namespace {
struct wayland_cursor_theme_wrapper {
	constexpr static auto cursor_size = 32;

	wayland_cursor_theme_wrapper(const wayland_shm_wrapper& wayland_shm) :
		theme(wl_cursor_theme_load(
			nullptr, //
			cursor_size,
			wayland_shm.shm
		))
	{
		if (!this->theme) {
			// no default theme
			return;
		}

		this->cursors[ruis::mouse_cursor::arrow] = wl_cursor_theme_get_cursor(this->theme, "left_ptr");
		this->cursors[ruis::mouse_cursor::top_left_corner] = wl_cursor_theme_get_cursor(this->theme, "top_left_corner");
		this->cursors[ruis::mouse_cursor::top_right_corner] =
			wl_cursor_theme_get_cursor(this->theme, "top_right_corner");
		this->cursors[ruis::mouse_cursor::bottom_left_corner] =
			wl_cursor_theme_get_cursor(this->theme, "bottom_left_corner");
		this->cursors[ruis::mouse_cursor::bottom_right_corner] =
			wl_cursor_theme_get_cursor(this->theme, "bottom_right_corner");
		this->cursors[ruis::mouse_cursor::top_side] = wl_cursor_theme_get_cursor(this->theme, "top_side");
		this->cursors[ruis::mouse_cursor::bottom_side] = wl_cursor_theme_get_cursor(this->theme, "bottom_side");
		this->cursors[ruis::mouse_cursor::left_side] = wl_cursor_theme_get_cursor(this->theme, "left_side");
		this->cursors[ruis::mouse_cursor::right_side] = wl_cursor_theme_get_cursor(this->theme, "right_side");
		this->cursors[ruis::mouse_cursor::grab] = wl_cursor_theme_get_cursor(this->theme, "grabbing");
		this->cursors[ruis::mouse_cursor::index_finger] = wl_cursor_theme_get_cursor(this->theme, "hand1");
		this->cursors[ruis::mouse_cursor::caret] = wl_cursor_theme_get_cursor(this->theme, "xterm");
	}

	wayland_cursor_theme_wrapper(const wayland_cursor_theme_wrapper&) = delete;
	wayland_cursor_theme_wrapper& operator=(const wayland_cursor_theme_wrapper&) = delete;

	wayland_cursor_theme_wrapper(wayland_cursor_theme_wrapper&&) = delete;
	wayland_cursor_theme_wrapper& operator=(wayland_cursor_theme_wrapper&&) = delete;

	~wayland_cursor_theme_wrapper()
	{
		if (this->theme) {
			wl_cursor_theme_destroy(this->theme);
		}
	}

	wl_cursor* get(ruis::mouse_cursor cursor)
	{
		return this->cursors[cursor];
	}

private:
	wl_cursor_theme* const theme;

	utki::enum_array<wl_cursor*, ruis::mouse_cursor> cursors = {nullptr};
};
} // namespace
