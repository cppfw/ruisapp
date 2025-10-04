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

#include "display.hxx"

#include <utki/string.hpp>

display_wrapper::sdl_wrapper::sdl_wrapper()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		throw std::runtime_error(utki::cat("Could not initialize SDL, SDL_Error: ", SDL_GetError()));
	}
}

display_wrapper::sdl_wrapper::~sdl_wrapper()
{
	SDL_Quit();
}

void display_wrapper::sdl_cursor_wrapper::set(ruis::mouse_cursor cursor)
{
	utki::assert(!this->sdl_cursor, SL);

	static constexpr utki::enum_array<SDL_SystemCursor, ruis::mouse_cursor> cursor_mapping = {
		SDL_SYSTEM_CURSOR_ARROW, // none
		SDL_SYSTEM_CURSOR_ARROW, // arrow
		SDL_SYSTEM_CURSOR_SIZEWE, // left_right_arrow
		SDL_SYSTEM_CURSOR_SIZENS, // up_down_arrow
		SDL_SYSTEM_CURSOR_SIZEALL, // all_directions_arrow
		SDL_SYSTEM_CURSOR_SIZEWE, // left_side
		SDL_SYSTEM_CURSOR_SIZEWE, // right_side
		SDL_SYSTEM_CURSOR_SIZENS, // top_side
		SDL_SYSTEM_CURSOR_SIZENS, // bottom_side
		SDL_SYSTEM_CURSOR_SIZEALL, // top_left_corner
		SDL_SYSTEM_CURSOR_SIZEALL, // top_right_corner
		SDL_SYSTEM_CURSOR_SIZEALL, // bottom_left_corner
		SDL_SYSTEM_CURSOR_SIZEALL, // bottom_right_corner
		SDL_SYSTEM_CURSOR_CROSSHAIR, // index_finger
		SDL_SYSTEM_CURSOR_HAND, // grab
		SDL_SYSTEM_CURSOR_IBEAM, // caret
		// TODO:
		// SDL_SYSTEM_CURSOR_WAIT
		// SDL_SYSTEM_CURSOR_NO
	};

	this->sdl_cursor = SDL_CreateSystemCursor(cursor_mapping[cursor]);
}

display_wrapper::sdl_cursor_wrapper::~sdl_cursor_wrapper()
{
	if (this->sdl_cursor) {
		SDL_FreeCursor(this->sdl_cursor);
	}
}

void display_wrapper::sdl_cursor_wrapper::activate()
{
	utki::assert(!this->empty(), SL);
	SDL_SetCursor(this->sdl_cursor);
}

void display_wrapper::set_cursor(ruis::mouse_cursor cursor)
{
	auto& c = this->mouse_cursors[cursor];
	if (c.empty()) {
		c.set(cursor);
	}
	c.activate();
}

void display_wrapper::set_mouse_cursor_visible(bool visible)
{
	int mode = [&]() {
		if (visible) {
			return SDL_ENABLE;
		} else {
			return SDL_DISABLE;
		}
	}();

	int error = SDL_ShowCursor(mode);

	if (error < 0) {
		throw std::runtime_error(utki::cat(
			"application::set_mouse_cursor_visible(): could not show/hide mouse cursor, error: ", //
			SDL_GetError()
		));
	}
}

display_wrapper::display_wrapper() :
	user_event_type_id([]() {
		Uint32 t = SDL_RegisterEvents(1);
		if (t == (Uint32)(-1)) {
			throw std::runtime_error(utki::cat(
				"Could not create SDL user event type, SDL Error: ", //
				SDL_GetError()
			));
		}
		return t;
	}())
{
	SDL_StartTextInput();
}

display_wrapper::~display_wrapper()
{
	SDL_StopTextInput();
}
