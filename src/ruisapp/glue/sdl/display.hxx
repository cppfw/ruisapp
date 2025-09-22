#pragma once

#include <stdexcept>

#include <ruis/util/mouse_cursor.hpp>
#include <utki/config.hpp>
#include <utki/enum_array.hpp>

#if CFG_COMPILER == CFG_COMPILER_MSVC
#	include <SDL.h>
#else
#	include <SDL2/SDL.h>
#endif

namespace {
class display_wrapper
{
	struct sdl_wrapper {
		sdl_wrapper();

		sdl_wrapper(const sdl_wrapper&) = delete;
		sdl_wrapper& operator=(const sdl_wrapper&) = delete;
		sdl_wrapper(sdl_wrapper&&) = delete;
		sdl_wrapper& operator=(sdl_wrapper&&) = delete;

		~sdl_wrapper();
	} sdl;

	class sdl_cursor_wrapper
	{
		SDL_Cursor* sdl_cursor = nullptr;

	public:
		sdl_cursor_wrapper() = default;

		sdl_cursor_wrapper(const sdl_cursor_wrapper&) = delete;
		sdl_cursor_wrapper& operator=(const sdl_cursor_wrapper&) = delete;

		sdl_cursor_wrapper(sdl_cursor_wrapper&&) = delete;
		sdl_cursor_wrapper& operator=(sdl_cursor_wrapper&&) = delete;

		void set(ruis::mouse_cursor cursor);

		~sdl_cursor_wrapper();

		bool empty() const noexcept
		{
			return this->sdl_cursor == nullptr;
		}

		void activate();
	};

	utki::enum_array<sdl_cursor_wrapper, ruis::mouse_cursor> mouse_cursors;

public:
	void set_cursor(ruis::mouse_cursor cursor);

	void set_mouse_cursor_visible(bool visible);

	const Uint32 user_event_type_id;

	display_wrapper();

	display_wrapper(const display_wrapper&) = delete;
	display_wrapper& operator=(const display_wrapper&) = delete;

	display_wrapper(display_wrapper&&) = delete;
	display_wrapper& operator=(display_wrapper&&) = delete;

	~display_wrapper();
};
} // namespace
