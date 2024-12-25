/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2024  Ivan Gagis <igagis@gmail.com>

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

#include <atomic>

#include <utki/config.hpp>

#include "../../application.hpp"

#if CFG_COMPILER == CFG_COMPILER_MSVC
#	include <SDL.h>
#else
#	include <SDL2/SDL.h>
#endif

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glew.h>
#	include <ruis/render/opengl/renderer.hpp>
#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <GLES2/gl2.h>
#	include <ruis/render/opengles/renderer.hpp>
#else
#	error "Unknown graphics API"
#endif

#include "../friend_accessors.cxx" // NOLINT(bugprone-suspicious-include)

using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
class window_wrapper : public utki::destructable
{
	class sdl_wrapper
	{
	public:
		sdl_wrapper()
		{
			if (SDL_Init(SDL_INIT_VIDEO) < 0) {
				throw std::runtime_error(utki::cat("Could not initialize SDL, SDL_Error: ", SDL_GetError()));
			}
		}

		~sdl_wrapper()
		{
			SDL_Quit();
		}

        sdl_wrapper(const sdl_wrapper&) = delete;
        sdl_wrapper& operator=(const sdl_wrapper&) = delete;
        sdl_wrapper(sdl_wrapper&&) = delete;
        sdl_wrapper& operator=(sdl_wrapper&&) = delete;
	} sdl;

public:
	class sdl_window_wrapper
	{
	public:
		SDL_Window* const window;

		sdl_window_wrapper(const window_params& wp) :
			window([&]() {
#ifdef RUISAPP_RENDER_OPENGL
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#elif defined(RUISAPP_RENDER_OPENGLES)
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
#	error "Unknown graphics API"
#endif
				{
					auto ver = wp.graphics_api_version;
					if (ver.major == 0 && ver.minor == 0) {
						// default OpenGL version is 2.0
						// TODO: set default version for non-OpenGL APIs
						ver.major = 2;
					}
					SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, ver.major);
					SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, ver.minor);
				}

				SDL_Window* window = SDL_CreateWindow(
					"SDL Tutorial",
					SDL_WINDOWPOS_UNDEFINED,
					SDL_WINDOWPOS_UNDEFINED,
					wp.dims.x(),
					wp.dims.y(),
					SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
				);
				if (!window) {
					std::runtime_error(utki::cat("Could not create SDL window, SDL_Error: ", SDL_GetError()));
				}
				return window;
			}())
		{}

		~sdl_window_wrapper()
		{
			SDL_DestroyWindow(this->window);
		}

        sdl_window_wrapper(const sdl_window_wrapper&) = delete;
        sdl_window_wrapper& operator=(const sdl_window_wrapper&) = delete;
        sdl_window_wrapper(sdl_window_wrapper&&) = delete;
        sdl_window_wrapper& operator=(sdl_window_wrapper&&) = delete;
	} window;

	class gl_context_wrapper
	{
		SDL_GLContext context;

	public:
		gl_context_wrapper(sdl_window_wrapper& sdl_window) :
			context([&]() {
				SDL_GLContext c = SDL_GL_CreateContext(sdl_window.window);
				if (!c) {
					throw std::runtime_error(utki::cat("Could not create OpenGL context, SDL Error: ", SDL_GetError()));
				}
				return c;
			}())
		{}

		~gl_context_wrapper()
		{
			SDL_GL_DeleteContext(this->context);
		}
	} gl_context;

	Uint32 user_event_type;

	std::atomic_bool quit_flag = false;

	window_wrapper(const window_params& wp) :
		window(wp),
		gl_context(this->window),
		user_event_type([]() {
			Uint32 t = SDL_RegisterEvents(1);
			if (t == (Uint32)(-1)) {
				throw std::runtime_error(
					utki::cat("Could not create SDL user event type, SDL Error: ", SDL_GetError())
				);
			}
			return t;
		}())
	{
#ifdef RUISAPP_RENDER_OPENGL
		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("Could not initialize GLEW");
		}
#endif
		SDL_StartTextInput();
	}

	~window_wrapper()
	{
		SDL_StopTextInput();
	}

    window_wrapper(const window_wrapper&) = delete;
    window_wrapper& operator=(const window_wrapper&) = delete;
    window_wrapper(window_wrapper&&) = delete;
    window_wrapper& operator=(window_wrapper&&) = delete;
};
} // namespace

namespace {
ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	ruisapp::application::directories dirs;

	// TODO:
	dirs.cache = utki::cat(".cache/"sv, app_name);
	dirs.config = utki::cat(".config/"sv, app_name);
	dirs.state = utki::cat(".local/state/"sv, app_name);

	// std::cout << "cache dir = " << dirs.cache << std::endl;
	// std::cout << "config dir = " << dirs.config << std::endl;
	// std::cout << "state dir = " << dirs.state << std::endl;

	return dirs;
}
} // namespace

namespace {
window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
{
	ASSERT(dynamic_cast<window_wrapper*>(pimpl.get()))
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
	return static_cast<window_wrapper&>(*pimpl);
}

window_wrapper& get_impl(application& app)
{
	return get_impl(get_window_pimpl(app));
}
} // namespace

namespace {
ruis::mouse_button button_number_to_enum(Uint8 number)
{
	switch (number) {
		case SDL_BUTTON_LEFT:
			return ruis::mouse_button::left;
		case SDL_BUTTON_X1:
			// TODO:
		case SDL_BUTTON_X2:
			// TODO:
		default:
		case SDL_BUTTON_MIDDLE:
			return ruis::mouse_button::middle;
		case SDL_BUTTON_RIGHT:
			return ruis::mouse_button::right;
	}
}
} // namespace

application::application(std::string name, const window_params& wp) :
	name(std::move(name)),
	window_pimpl(std::make_unique<window_wrapper>(wp)),
	gui(utki::make_shared<ruis::context>(
#ifdef RUISAPP_RENDER_OPENGL
		utki::make_shared<ruis::render::opengl::renderer>(),
#elif defined(RUISAPP_RENDER_OPENGLES)
		utki::make_shared<ruis::render::opengles::renderer>(),
#else
#	error "Unknown graphics API"
#endif
		utki::make_shared<ruis::updater>(),
		[this](std::function<void()> procedure) {
			auto& ww = get_impl(*this);

			SDL_Event e;
			SDL_memset(&e, 0, sizeof(e));
			e.type = ww.user_event_type;
			e.user.code = 0;
			e.user.data1 = new std::function<void()>(std::move(procedure));
			e.user.data2 = 0;
			SDL_PushEvent(&e);
		},
		[this](ruis::mouse_cursor c) {
			// TODO:
			// auto& ww = get_impl(*this);
			// ww.set_cursor(c);
		},
		// TODO:
		96, // get_impl(window_pimpl).get_dots_per_inch(),
		1 // get_impl(window_pimpl).get_dots_per_pp()
	)),
	directory(get_application_directories(this->name))
{
#ifdef RUISAPP_RASPBERRYPI
	this->set_fullscreen(true);
#else
	this->update_window_rect(ruis::rect(0, 0, ruis::real(wp.dims.x()), ruis::real(wp.dims.y())));
#endif
}

void application::quit() noexcept
{
	auto& ww = get_impl(this->window_pimpl);

	ww.quit_flag.store(true);
}

void application::swap_frame_buffers()
{
	auto& ww = get_impl(this->window_pimpl);

	SDL_GL_SwapWindow(ww.window.window);
}

void application::set_fullscreen(bool enable)
{
	// TODO:
}

void application::set_mouse_cursor_visible(bool visible)
{
	// TODO:
}

int main(int argc, const char** argv)
{
	std::unique_ptr<ruisapp::application> app = ruisapp::application_factory::create_application(argc, argv);
	if (!app) {
		return 1;
	}

	ASSERT(app)

	auto& ww = get_impl(*app);

	while (!ww.quit_flag.load()) {
		// sequence:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle

		auto to_wait_ms = app->gui.update();

		render(*app);

		if (SDL_WaitEventTimeout(nullptr, to_wait_ms) == 0) {
			// No events or error. In case of error not much we can do, just ignore it.
			continue;
		}

		ruis::vector2 new_win_dims(-1, -1);

		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			switch (e.type) {
				case SDL_QUIT:
					ww.quit_flag.store(true);
					break;
				case SDL_WINDOWEVENT:
					switch (e.window.event) {
						default:
							break;
						case SDL_WINDOWEVENT_RESIZED:
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							// squash all window resize events into one, for that store the new
							// window dimensions and update the viewport later only once
							new_win_dims.x() = ruis::real(e.window.data1);
							new_win_dims.y() = ruis::real(e.window.data2);
							break;
						case SDL_WINDOWEVENT_ENTER:
							handle_mouse_hover(*app, true, 0);
							break;
						case SDL_WINDOWEVENT_LEAVE:
							handle_mouse_hover(*app, false, 0);
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					{
						int x = 0;
						int y = 0;
						SDL_GetMouseState(&x, &y);

						handle_mouse_move(*app, ruis::vector2(x, y), 0);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					[[fallthrough]];
				case SDL_MOUSEBUTTONUP:
					{
						int x = 0;
						int y = 0;
						SDL_GetMouseState(&x, &y);

						handle_mouse_button(
							*app,
							e.button.type == SDL_MOUSEBUTTONDOWN,
							ruis::vector2(x, y),
							button_number_to_enum(e.button.button),
							0 // pointer id
						);
					}
					break;
				case SDL_KEYDOWN:
					[[fallthrough]];
				case SDL_KEYUP:
					// if (e.key.repeat == 0) {
					// 	gui.send_key(e.key.type == SDL_KEYDOWN, sdl_scan_code_to_ruis_key(e.key.keysym.scancode));
					// }
					// if (e.type == SDL_KEYDOWN) {
					// 	struct SDLUnicodeDummyProvider : public ruis::gui::input_string_provider {
					// 		std::u32string get() const override
					// 		{
					// 			return std::u32string();
					// 		}
					// 	};

					// 	gui.send_character_input(
					// 		SDLUnicodeDummyProvider(),
					// 		sdl_scan_code_to_ruis_key(e.key.keysym.scancode)
					// 	);
					// }
					break;
				case SDL_TEXTINPUT:
					// {
					// 	struct SDLUnicodeProvider : public ruis::gui::input_string_provider {
					// 		const char* text;

					// 		SDLUnicodeProvider(const char* text) :
					// 			text(text)
					// 		{}

					// 		std::u32string get() const override
					// 		{
					// 			return utki::to_utf32(this->text);
					// 		}
					// 	} sdlUnicodeProvider(
					// 		// save pointer to text, the ownership of text buffer is not taken!
					// 		e.text.text
					// 	);

					// 	gui.send_character_input(sdlUnicodeProvider, ruis::key::unknown);
					// }
					break;
				default:
					if (e.type == ww.user_event_type) {
						std::unique_ptr<std::function<void()>> f(
							reinterpret_cast<std::function<void()>*>(e.user.data1)
						);
						f->operator()();
					}
					break;
			}
		}

		if (new_win_dims.is_positive_or_zero()) {
			update_window_rect(*app, ruis::rect(0, new_win_dims));
		}
	}

	return 0;
}
