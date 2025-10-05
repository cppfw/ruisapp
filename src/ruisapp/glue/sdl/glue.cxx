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

#include <atomic>

#include <utki/config.hpp>
#include <utki/enum_array.hpp>
#include <utki/unicode.hpp>

#include "application.hxx"
#include "key_code_map.hxx"

// include implementations
#include "application.cxx" // NOLINT(bugprone-suspicious-include, "not suspicious")
#include "display.cxx" // NOLINT(bugprone-suspicious-include, "not suspicious")
#include "window.cxx" // NOLINT(bugprone-suspicious-include, "not suspicious")

using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
ruis::key sdl_scancode_to_ruis_key(SDL_Scancode sc)
{
	if (size_t(sc) >= key_code_map.size()) {
		return ruis::key::unknown;
	}

	return key_code_map[sc];
}
} // namespace

namespace {
ruis::mouse_button sdl_button_number_to_ruis_enum(Uint8 number)
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

namespace {
void main_loop_iteration(void* user_data)
{
	utki::assert(user_data, SL);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto app = reinterpret_cast<ruisapp::application*>(user_data);

	auto& glue = get_glue(*app);

	glue.windows_to_destroy.clear();

	// loop iteration sequence:
	// - update updateables
	// - render
	// - wait for events and handle them/next cycle

#if CFG_OS_NAME != CFG_OS_NAME_EMSCRIPTEN
	auto to_wait_ms =
#endif
		glue.updater.get().update();

	glue.render();

#if CFG_OS_NAME != CFG_OS_NAME_EMSCRIPTEN
	// clamp to_wait_ms to max of int as SDL_WaitEventTimeout() accepts int type
	to_wait_ms = std::min(to_wait_ms, uint32_t(std::numeric_limits<int32_t>::max()));

	if (SDL_WaitEventTimeout(nullptr, int(to_wait_ms)) == 0) {
		// No events or error. In case of error not much we can do, just ignore it.
		return;
	}
#endif

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0) {
		switch (e.type) {
			case SDL_QUIT:
				glue.quit_flag.store(true);
				break;
			case SDL_WINDOWEVENT:
				if (auto window = glue.get_window(e.window.windowID)) {
					auto& win = *window;
					auto& natwin = win.ruis_native_window.get();
					switch (e.window.event) {
						default:
							break;
						case SDL_WINDOWEVENT_RESIZED:
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							// squash all window resize events into one, for that store the new
							// window dimensions and update the viewport later only once
							win.new_win_dims.x() = ruis::real(e.window.data1);
							win.new_win_dims.y() = ruis::real(e.window.data2);
#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
							win.new_win_dims *= natwin.get_scale_factor();
#endif
							// std::cout << "new window dims = " << new_win_dims << std::endl;
							break;
						case SDL_WINDOWEVENT_ENTER:
							natwin.set_hovered(true);
							win.gui.send_mouse_hover(
								true, //
								0 // pointer id
							);
							break;
						case SDL_WINDOWEVENT_LEAVE:
							natwin.set_hovered(false);
							win.gui.send_mouse_hover(
								false, //
								0 // pointer id
							);
							break;
						case SDL_WINDOWEVENT_CLOSE:
							if (natwin.close_handler) {
								natwin.close_handler();
							}
							break;
					}
				}
				break;
			case SDL_MOUSEMOTION:
				if (auto window = glue.get_window(e.motion.windowID)) {
					auto& win = *window;

					int x = 0;
					int y = 0;
					SDL_GetMouseState(&x, &y);

					ruis::vector2 pos(x, y);

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
					auto& natwin = win.ruis_native_window.get();
					pos *= natwin.get_scale_factor();
#endif

					win.gui.send_mouse_move(
						pos, //
						0 // pointer id
					);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				[[fallthrough]];
			case SDL_MOUSEBUTTONUP:
				if (auto window = glue.get_window(e.button.windowID)) {
					auto& win = *window;

					int x = 0;
					int y = 0;
					SDL_GetMouseState(&x, &y);

					ruis::vector2 pos(x, y);

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
					auto& natwin = win.ruis_native_window.get();
					pos *= natwin.get_scale_factor();
#endif

					win.gui.send_mouse_button(
						e.button.type == SDL_MOUSEBUTTONDOWN,
						pos,
						sdl_button_number_to_ruis_enum(e.button.button),
						0 // pointer id
					);
				}
				break;
			case SDL_KEYDOWN:
				[[fallthrough]];
			case SDL_KEYUP:
				if (auto window = glue.get_window(e.key.windowID)) {
					auto& win = *window;

					auto key = sdl_scancode_to_ruis_key(e.key.keysym.scancode);
					if (e.key.repeat == 0) {
						win.gui.send_key(
							e.key.type == SDL_KEYDOWN, //
							key
						);
					}
					if (e.type == SDL_KEYDOWN) {
						struct sdl_dummy_input_string_provider : public ruis::gui::input_string_provider {
							std::u32string get() const override
							{
								return {};
							}
						};

						win.gui.send_character_input(
							sdl_dummy_input_string_provider(), //
							key
						);
					}
				}
				break;
			case SDL_TEXTINPUT:
				if (auto window = glue.get_window(e.text.windowID)) {
					auto& win = *window;

					struct sdl_input_string_provider : public ruis::gui::input_string_provider {
						const char* text;

						sdl_input_string_provider(const char* text) :
							text(text)
						{}

						std::u32string get() const override
						{
							return utki::to_utf32(this->text);
						}
					} sdl_input_string_provider(
						// save pointer to text, the ownership of text buffer is not taken!
						&(e.text.text[0])
					);

					win.gui.send_character_input(
						sdl_input_string_provider, //
						ruis::key::unknown
					);
				}
				break;
			default:
				if (e.type == glue.display.get().user_event_type_id) {
					std::unique_ptr<std::function<void()>> f(
						// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
						reinterpret_cast<std::function<void()>*>(e.user.data1)
					);
					f->operator()();
				}
				break;
		}
	}

	glue.apply_new_win_dims();

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
	if (glue.quit_flag.load()) {
		std::unique_ptr<ruisapp::application> p(app);
		emscripten_cancel_main_loop();
	}
#endif
}
} // namespace

int main(int argc, const char** argv) noexcept(false)
{
	try {
		// std::cout << "main(): enter" << std::endl;
		auto app = ruisapp::application_factory::make_application(argc, argv);
		if (!app) {
			// Not an error. The application just did not show any GUI to the user.
			return 0;
		}
		// std::cout << "main(): app created" << std::endl;

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
		emscripten_set_main_loop_arg(
			&main_loop_iteration, // iteration callback
			app.release(), // user data
			0, // fps, 0=vsync
			false // false = do not simulate infinite loop (throwing an exception)
		);
		// std::cout << "main(): emscripten loop is set up" << std::endl;
		return 0;
#else
		auto& glue = get_glue(*app);

		while (!glue.quit_flag.load()) {
			main_loop_iteration(app.get());
		}
#endif
	} catch (std::exception& e) {
#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
		std::cout << "uncaught " << utki::to_string(e) << std::endl;
#endif
		throw;
	}

	return 0;
}
