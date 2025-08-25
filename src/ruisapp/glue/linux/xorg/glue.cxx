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

#include <array>
#include <string_view>
#include <vector>

#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <papki/fs_file.hpp>
#include <utki/string.hpp>
#include <utki/unicode.hpp>

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glew.h>
#	include <ruis/render/opengl/context.hpp>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <GLES2/gl2.h>

#	include <ruis/render/opengles/context.hpp>

#else
#	error "Unknown graphics API"
#endif

#include "../../../application.hpp"

// TODO: make hxx
#include "../../friend_accessors.cxx" // NOLINT(bugprone-suspicious-include)
#include "../../unix_common.cxx" // NOLINT(bugprone-suspicious-include)

#include "cursor.hxx"
#include "display.hxx"
#include "window.hxx"

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
class os_platform_glue : public utki::destructable
{
public:
	const utki::shared_ref<display_wrapper> display = utki::make_shared<display_wrapper>();

	nitki::queue ui_queue;

	std::atomic_bool quit_flag = false;
};
} // namespace

namespace {
os_platform_glue& get_glue(ruisapp::application& app)
{
	return static_cast<os_platform_glue&>(app.pimpl.get());
}
} // namespace

namespace {
struct window_wrapper : public utki::destructable {
	utki::shared_ref<display_wrapper> display;

	utki::shared_ref<native_window> window;

	window_wrapper(
		const utki::version_duplet& gl_version, //
		const ruisapp::window_parameters& window_params,
		utki::shared_ref<display_wrapper> display
	) :
		display(std::move(display)),
		window(utki::make_shared<native_window>(
			this->display,
			gl_version,
			window_params //
		))
	{
#ifdef RUISAPP_RENDER_OPENGL
#elif defined(RUISAPP_RENDER_OPENGLES)
#else
#	error "Unknown graphics API"
#endif
	}

	window_wrapper(const window_wrapper&) = delete;
	window_wrapper& operator=(const window_wrapper&) = delete;

	window_wrapper(window_wrapper&&) = delete;
	window_wrapper& operator=(window_wrapper&&) = delete;

	~window_wrapper() override {}
};

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

application::application(parameters params) :
	pimpl(utki::make_unique<os_platform_glue>()),
	name(std::move(params.name)),
	window_pimpl(std::make_unique<window_wrapper>(
		params.graphics_api_version, //
		// TODO: check that there is at least 1 window
		params.windows.front(),
		[&]() {
			auto& glue = get_glue(*this);
			return glue.display;
		}()
	)),
	gui(utki::make_shared<ruis::context>(
		utki::make_shared<ruis::style_provider>( //
			utki::make_shared<ruis::resource_loader>( //
				utki::make_shared<ruis::render::renderer>(
#ifdef RUISAPP_RENDER_OPENGL
					utki::make_shared<ruis::render::opengl::context>(get_impl(*this).window)
#elif defined(RUISAPP_RENDER_OPENGLES)
					utki::make_shared<ruis::render::opengles::context>(get_impl(*this).window)
#else
#	error "Unknown graphics API"
#endif
				)
			)
		),
		utki::make_shared<ruis::updater>(),
		ruis::context::parameters{
			.post_to_ui_thread_function =
				[this](std::function<void()> proc) {
					auto& glue = get_glue(*this);
					glue.ui_queue.push_back(std::move(proc));
				},
			.units =
				[this]() {
					auto& glue = get_glue(*this);
					auto& display = glue.display.get();
					return ruis::units(
						display.get_dots_per_inch(), //
						display.get_dots_per_pp()
					);
				}()
		}
	)),
	directory(get_application_directories(this->name))
{
	this->update_window_rect(ruis::rect(
		0, //
		0,
		ruis::real(params.windows.front().dims.x()),
		ruis::real(params.windows.front().dims.y())
	));
}

namespace {

class xevent_waitable : public opros::waitable
{
public:
	xevent_waitable(Display* d) :
		opros::waitable(XConnectionNumber(d))
	{}
};

ruis::mouse_button button_number_to_enum(unsigned number)
{
	switch (number) {
		case 1: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::left;
		default:
		case 2: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::middle;
		case 3: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::right;
		case 4: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_up;
		case 5: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_down;
		case 6: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_left;
		case 7: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_right;
	}
}

const std::array<ruis::key, size_t(std::numeric_limits<uint8_t>::max()) + 1> key_code_map = {
	{//
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::escape, // 9
	 ruis::key::one, // 10
	 ruis::key::two, // 11
	 ruis::key::three, // 12
	 ruis::key::four, // 13
	 ruis::key::five, // 14
	 ruis::key::six, // 15
	 ruis::key::seven, // 16
	 ruis::key::eight, // 17
	 ruis::key::nine, // 18
	 ruis::key::zero, // 19
	 ruis::key::minus, // 20
	 ruis::key::equals, // 21
	 ruis::key::backspace, // 22
	 ruis::key::tabulator, // 23
	 ruis::key::q, // 24
	 ruis::key::w, // 25
	 ruis::key::e, // 26
	 ruis::key::r, // 27
	 ruis::key::t, // 28
	 ruis::key::y, // 29
	 ruis::key::u, // 30
	 ruis::key::i, // 31
	 ruis::key::o, // 32
	 ruis::key::p, // 33
	 ruis::key::left_square_bracket, // 34
	 ruis::key::right_square_bracket, // 35
	 ruis::key::enter, // 36
	 ruis::key::left_control, // 37
	 ruis::key::a, // 38
	 ruis::key::s, // 39
	 ruis::key::d, // 40
	 ruis::key::f, // 41
	 ruis::key::g, // 42
	 ruis::key::h, // 43
	 ruis::key::j, // 44
	 ruis::key::k, // 45
	 ruis::key::l, // 46
	 ruis::key::semicolon, // 47
	 ruis::key::apostrophe, // 48
	 ruis::key::grave, // 49
	 ruis::key::left_shift, // 50
	 ruis::key::backslash, // 51
	 ruis::key::z, // 52
	 ruis::key::x, // 53
	 ruis::key::c, // 54
	 ruis::key::v, // 55
	 ruis::key::b, // 56
	 ruis::key::n, // 57
	 ruis::key::m, // 58
	 ruis::key::comma, // 59
	 ruis::key::period, // 60
	 ruis::key::slash, // 61
	 ruis::key::right_shift, // 62
	 ruis::key::unknown,
	 ruis::key::left_alt, // 64
	 ruis::key::space, // 65
	 ruis::key::capslock, // 66
	 ruis::key::f1, // 67
	 ruis::key::f2, // 68
	 ruis::key::f3, // 69
	 ruis::key::f4, // 70
	 ruis::key::f5, // 71
	 ruis::key::f6, // 72
	 ruis::key::f7, // 73
	 ruis::key::f8, // 74
	 ruis::key::f9, // 75
	 ruis::key::f10, // 76
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::f11, // 95
	 ruis::key::f12, // 96
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::right_control, // 105
	 ruis::key::unknown,
	 ruis::key::print_screen, // 107
	 ruis::key::right_alt, // 108
	 ruis::key::unknown,
	 ruis::key::home, // 110
	 ruis::key::arrow_up, // 111
	 ruis::key::page_up, // 112
	 ruis::key::arrow_left, // 113
	 ruis::key::arrow_right, // 114
	 ruis::key::end, // 115
	 ruis::key::arrow_down, // 116
	 ruis::key::page_down, // 117
	 ruis::key::insert, // 118
	 ruis::key::deletion, // 119
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::pause, // 127
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::left_command, // 133
	 ruis::key::unknown,
	 ruis::key::menu, // 135
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown
	}
};

class key_event_unicode_provider : public ruis::gui::input_string_provider
{
	const native_window& window;
	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	XKeyEvent& event;

public:
	key_event_unicode_provider(
		const native_window& window, //
		XKeyEvent& event
	) :
		window(window),
		event(event)
	{}

	std::u32string get() const override
	{
		return this->window.get_string(this->event);
	}
};

} // namespace

void application::quit() noexcept
{
	auto& glue = get_glue(*this);
	glue.quit_flag.store(true);
}

int main(int argc, const char** argv)
{
	std::unique_ptr<ruisapp::application> app = create_app_unix(argc, argv);
	if (!app) {
		// Not an error. The app just did not show any GUI to the user.
		return 0;
	}
	utki::assert(app, SL);

	auto& glue = get_glue(*app);

	auto& ww = get_impl(get_window_pimpl(*app));

	xevent_waitable xew(ww.display.get().xorg_display.display);

	opros::wait_set wait_set(2);

	wait_set.add(xew, {opros::ready::read}, &xew);
	utki::scope_exit xew_wait_set_scope_exit([&]() {
		wait_set.remove(xew);
	});

	wait_set.add(glue.ui_queue, {opros::ready::read}, &glue.ui_queue);
	utki::scope_exit ui_queue_wait_set_scope_exit([&]() {
		wait_set.remove(glue.ui_queue);
	});

	// Sometimes the first Expose event does not come for some reason. It happens
	// constantly in some systems and never happens on all the others. So, render
	// everything for the first time.
	render(*app);

	while (!glue.quit_flag.load()) {
		// sequence:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle
		auto to_wait_ms = app->gui.update();
		render(*app);
		wait_set.wait(to_wait_ms);

		auto triggered_events = wait_set.get_triggered();

		bool ui_queue_ready_to_read = false;

		for (auto& ei : triggered_events) {
			if (ei.user_data == &glue.ui_queue) {
				ui_queue_ready_to_read = true;
			}
		}

		if (ui_queue_ready_to_read) {
			while (auto m = glue.ui_queue.pop_front()) {
				utki::log_debug([](auto& o) {
					o << "loop message" << std::endl;
				});
				m();
			}
		}

		ruis::vector2 new_win_dims(-1, -1);

		// NOTE: do not check 'read' flag for X event, for some reason when waiting
		// with 0 timeout it will never be set.
		//       Maybe some bug in XWindows, maybe something else.
		bool x_event_arrived = false;
		while (XPending(ww.display.get().xorg_display.display) > 0) {
			x_event_arrived = true;
			XEvent event;
			XNextEvent(ww.display.get().xorg_display.display, &event);
			// TRACE(<< "X event got, type = " << (event.type) << std::endl)
			switch (event.type) {
				case Expose:
					//						TRACE(<< "Expose X event
					// got" << std::endl)
					if (event.xexpose.count != 0) {
						break;
					}
					render(*app);
					break;
				case ConfigureNotify:
					// squash all window resize events into one, for that store the new
					// window dimensions and update the viewport later only once
					new_win_dims.x() = ruis::real(event.xconfigure.width);
					new_win_dims.y() = ruis::real(event.xconfigure.height);
					break;
				case KeyPress:
					//						TRACE(<< "KeyPress X
					// event got" << std::endl)
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						ruis::key key = key_code_map[std::uint8_t(event.xkey.keycode)];
						handle_key_event(*app, true, key);
						handle_character_input(*app, key_event_unicode_provider(ww.window, event.xkey), key);
					}
					break;
				case KeyRelease:
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						ruis::key key = key_code_map[std::uint8_t(event.xkey.keycode)];

						// detect auto-repeated key events
						if (XEventsQueued(
								ww.display.get().xorg_display.display, //
								QueuedAfterReading
							))
						{ // if there are other events queued
							XEvent nev;
							XPeekEvent(
								ww.display.get().xorg_display.display, //
								&nev
							);

							if (nev.type == KeyPress && nev.xkey.time == event.xkey.time &&
								nev.xkey.keycode == event.xkey.keycode)
							{
								// key wasn't actually released
								handle_character_input(
									*app, //
									key_event_unicode_provider(ww.window, nev.xkey),
									key
								);

								XNextEvent(
									ww.display.get().xorg_display.display,
									&nev
								); // remove the key down event from queue
								break;
							}
						}

						handle_key_event(*app, false, key);
					}
					break;
				case ButtonPress:
					// utki::log_debug([&](auto&o){o << "ButtonPress X event got, button mask = " <<
					// event.xbutton.button << std::endl;}) utki::log_debug([&](auto&o){o <<
					// "ButtonPress X event got, x, y = " << event.xbutton.x << ", "
					// << event.xbutton.y << std::endl;})
					handle_mouse_button(
						*app,
						true,
						ruis::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case ButtonRelease:
					// utki::log_debug([&](auto&o){o << "ButtonRelease X event got, button mask = " <<
					// event.xbutton.button << std::endl;})
					handle_mouse_button(
						*app,
						false,
						ruis::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case MotionNotify:
					//						TRACE(<< "MotionNotify X
					// event got" << std::endl)
					handle_mouse_move(*app, ruis::vector2(event.xmotion.x, event.xmotion.y), 0);
					break;
				case EnterNotify:
					handle_mouse_hover(*app, true, 0);
					break;
				case LeaveNotify:
					handle_mouse_hover(*app, false, 0);
					break;
				case ClientMessage:
					//						TRACE(<< "ClientMessage
					// X event got" << std::endl)

					// probably a WM_DELETE_WINDOW event
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
						char* name = XGetAtomName(ww.display.get().xorg_display.display, event.xclient.message_type);
						if ("WM_PROTOCOLS"sv == name) {
							glue.quit_flag.store(true);
						}
						XFree(name);
					}
					break;
				default:
					// ignore
					break;
			}
		}

		// WORKAROUND: XEvent file descriptor becomes ready to read many times per
		// second, even if
		//             there are no events to handle returned by XPending(), so here
		//             we check if something meaningful actually happened and call
		//             render() only if it did
		if (triggered_events.size() != 0 && !x_event_arrived && !ui_queue_ready_to_read) {
			continue;
		}

		if (new_win_dims.is_positive_or_zero()) {
			update_window_rect(*app, ruis::rect(0, new_win_dims));
		}
	}

	return 0;
}

void application::set_fullscreen(bool enable)
{
	auto& ww = get_impl(this->window_pimpl);

	ww.window.get().set_fullscreen(enable);
}

bool application::is_fullscreen() const noexcept
{
	auto& ww = get_impl(this->window_pimpl);

	return ww.window.get().is_fullscreen();
}

void application::swap_frame_buffers()
{
	auto& ww = get_impl(this->window_pimpl);
	ww.window.get().swap_frame_buffers();
}
