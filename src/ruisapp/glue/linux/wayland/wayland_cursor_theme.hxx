#pragma once

#include <ruis/util/mouse_cursor.hpp>
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

		this->cursors[size_t(ruis::mouse_cursor::arrow)] = wl_cursor_theme_get_cursor(this->theme, "left_ptr");
		this->cursors[size_t(ruis::mouse_cursor::top_left_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "top_left_corner");
		this->cursors[size_t(ruis::mouse_cursor::top_right_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "top_right_corner");
		this->cursors[size_t(ruis::mouse_cursor::bottom_left_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "bottom_left_corner");
		this->cursors[size_t(ruis::mouse_cursor::bottom_right_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "bottom_right_corner");
		this->cursors[size_t(ruis::mouse_cursor::top_side)] = wl_cursor_theme_get_cursor(this->theme, "top_side");
		this->cursors[size_t(ruis::mouse_cursor::bottom_side)] = wl_cursor_theme_get_cursor(this->theme, "bottom_side");
		this->cursors[size_t(ruis::mouse_cursor::left_side)] = wl_cursor_theme_get_cursor(this->theme, "left_side");
		this->cursors[size_t(ruis::mouse_cursor::right_side)] = wl_cursor_theme_get_cursor(this->theme, "right_side");
		this->cursors[size_t(ruis::mouse_cursor::grab)] = wl_cursor_theme_get_cursor(this->theme, "grabbing");
		this->cursors[size_t(ruis::mouse_cursor::index_finger)] = wl_cursor_theme_get_cursor(this->theme, "hand1");
		this->cursors[size_t(ruis::mouse_cursor::caret)] = wl_cursor_theme_get_cursor(this->theme, "xterm");
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
		auto index = size_t(cursor);
		utki::assert(index < this->cursors.size(), SL);
		return this->cursors[index];
	}

private:
	wl_cursor_theme* const theme;

	// TODO: use utki::enum_array
	std::array<wl_cursor*, size_t(ruis::mouse_cursor::enum_size)> cursors = {nullptr};
};
} // namespace
