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

#include <ruis/config.hpp>
#include <ruis/render/native_window.hpp>
#include <utki/config.hpp>
#include <utki/shared_ref.hpp>
#include <utki/version.hpp>

#include "../../window.hpp"

#include "display.hxx"

namespace {
class native_window : public ruis::render::native_window
{
#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
	static bool on_emscripten_canvas_size_changed_callback(
		int event_type, //
		const void* reserved,
		void* user_data
	);
#endif

	utki::shared_ref<display_wrapper> display;

	struct sdl_window_wrapper {
		const ruis::real dpi;
		const ruis::real scale_factor;

		SDL_Window* const window;

		sdl_window_wrapper(
			const utki::version_duplet& gl_version, //
			const ruisapp::window_parameters& window_params,
			bool is_shared_context_window
		);

		sdl_window_wrapper(const sdl_window_wrapper&) = delete;
		sdl_window_wrapper& operator=(const sdl_window_wrapper&) = delete;
		sdl_window_wrapper(sdl_window_wrapper&&) = delete;
		sdl_window_wrapper& operator=(sdl_window_wrapper&&) = delete;

		~sdl_window_wrapper();
	} sdl_window;

	struct sdl_gl_context_wrapper {
		const SDL_GLContext context;

		sdl_gl_context_wrapper(sdl_window_wrapper& sdl_window);

		sdl_gl_context_wrapper(const sdl_gl_context_wrapper&) = delete;
		sdl_gl_context_wrapper& operator=(const sdl_gl_context_wrapper&) = delete;
		sdl_gl_context_wrapper(sdl_gl_context_wrapper&&) = delete;
		sdl_gl_context_wrapper& operator=(sdl_gl_context_wrapper&&) = delete;

		~sdl_gl_context_wrapper();
	} sdl_gl_context;

	bool mouse_cursor_visible = true;

	bool is_hovered = false;

public:
	using window_id_type = Uint32;

	window_id_type get_id() const
	{
		auto id = SDL_GetWindowID(this->sdl_window.window);
		if (id == 0) {
			throw std::runtime_error(utki::cat(
				"SDL_GetWindowID() failed: ", //
				SDL_GetError()
			));
		}
		return id;
	}

	native_window(
		utki::shared_ref<display_wrapper> display, //
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	);

	void swap_frame_buffers() override
	{
		SDL_GL_SwapWindow(this->sdl_window.window);
	}

	void bind_rendering_context() override
	{
		if (SDL_GL_MakeCurrent(
				this->sdl_window.window, //
				this->sdl_gl_context.context
			) != 0)
		{
			throw std::runtime_error(utki::cat(
				"SDL_GL_MakeCurrent() failed: ", //
				SDL_GetError()
			));
		}
	}

	ruis::real get_dpi() const noexcept
	{
		return this->sdl_window.dpi;
	}

	ruis::real get_scale_factor() const noexcept
	{
		return this->sdl_window.scale_factor;
	}

	ruis::vec2 get_dims() const noexcept;

	void set_mouse_cursor(ruis::mouse_cursor c) override;

	void set_hovered(bool hovered)
	{
		if (!this->is_hovered) {
			if (hovered) {
				this->display.get().set_mouse_cursor_visible(this->mouse_cursor_visible);
			}
		}

		this->is_hovered = hovered;
	}

	void set_mouse_cursor_visible(bool visible) override
	{
		this->mouse_cursor_visible = visible;

		if (this->is_hovered) {
			this->display.get().set_mouse_cursor_visible(this->mouse_cursor_visible);
		}
	}

	void set_fullscreen_internal(bool enable) override
	{
		auto flags = [&]() -> uint32_t {
			if (enable) {
				return SDL_WINDOW_FULLSCREEN_DESKTOP;
			} else {
				return 0;
			}
		}();

		auto error = SDL_SetWindowFullscreen(
			this->sdl_window.window, //
			flags
		);

		if (error != 0) {
			throw std::runtime_error(utki::cat(
				"SDL_SetWindowFullscreen() failed: ", //
				SDL_GetError()
			));
		}
	}
};
} // namespace
