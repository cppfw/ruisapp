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

#include <ratio>

#include <Shlobj.h> // needed for SHGetFolderPathA()
#include <papki/fs_file.hpp>
#include <ruis/context.hpp>
#include <ruis/util/util.hpp>
#include <utki/destructable.hpp>
#include <utki/windows.hpp>
#include <windowsx.h> // needed for GET_X_LPARAM macro and other similar macros

#ifdef RUISAPP_RENDER_OPENGL
#	include <ruis/render/opengl/context.hpp>
#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <ruis/render/opengles/context.hpp>
#else
#	error "Unknown graphics API"
#endif

#include "../../application.hpp"

// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "../friend_accessors.cxx"

using namespace ruisapp;

namespace {
constexpr const char* window_class_name = "RuisappWindowClassName";
} // namespace

namespace {
struct window_wrapper : public utki::destructable {
	HWND hwnd;
	HDC hdc;

#ifdef RUISAPP_RENDER_OPENGL
	HGLRC hrc;
#elif defined(RUISAPP_RENDER_OPENGLES)
	EGLDisplay egl_display;
	EGLSurface egl_surface;
	EGLContext egl_context;
#else
#	error "Unknown graphics API"
#endif

	bool quitFlag = false;

	bool isHovered = false; // for tracking when mouse enters or leaves window.

	utki::flags<ruis::mouse_button> mouseButtonState;

	bool mouseCursorIsCurrentlyVisible = true;

	window_wrapper(const window_parameters& wp);

	window_wrapper(const window_wrapper&) = delete;
	window_wrapper& operator=(const window_wrapper&) = delete;

	window_wrapper(window_wrapper&&) = delete;
	window_wrapper& operator=(window_wrapper&&) = delete;

	~window_wrapper() override;
};

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
{
	ASSERT(dynamic_cast<window_wrapper*>(pimpl.get()))
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
	return static_cast<window_wrapper&>(*pimpl);
}
} // namespace

namespace {
const std::array<ruis::key, std::numeric_limits<uint8_t>::max() + 1> key_code_map = {
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // VK_LBUTTON
	ruis::key::unknown, // VK_RBUTTON
	ruis::key::unknown, // VK_CANCEL
	ruis::key::unknown, // VK_MBUTTON
	ruis::key::unknown, // VK_XBUTTON1, 5
	ruis::key::unknown, // VK_XBUTTON2
	ruis::key::unknown, // Undefined
	ruis::key::backspace, // VK_BACK = backspace key
	ruis::key::tabulator, // VK_TAB
	ruis::key::unknown, // Reserved, 10
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // VK_CLEAR = clear key???
	ruis::key::enter, // VK_RETURN
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // Undefined, 15
	ruis::key::left_shift, // VK_SHIFT
	ruis::key::left_control, // VK_CONTROL
	ruis::key::left_alt, // VK_MENU = alt key
	ruis::key::pause, // VK_PAUSE
	ruis::key::capslock, // VK_CAPITAL = caps lock key, 20
	ruis::key::unknown, // VK_KANA, VK_HANGUEL, VK_HANGUL
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // VK_JUNJA
	ruis::key::unknown, // VK_FINAL
	ruis::key::unknown, // VK_HANJA, VK_KANJI, 25
	ruis::key::unknown, // Undefined
	ruis::key::escape, // VK_ESCAPE
	ruis::key::unknown, // VK_CONVERT
	ruis::key::unknown, // VK_NONCONVERT
	ruis::key::unknown, // VK_ACCEPT, 30
	ruis::key::unknown, // VK_MODECHANGE
	ruis::key::space, // VK_SPACE = space bar key
	ruis::key::page_up, // VK_PRIOR = page up key
	ruis::key::page_down, // VK_NEXT = page down key
	ruis::key::end, // VK_END, 35
	ruis::key::home, // VK_HOME
	ruis::key::arrow_left, // VK_LEFT
	ruis::key::arrow_up, // VK_UP
	ruis::key::arrow_right, // VK_RIGHT
	ruis::key::arrow_down, // VK_DOWN, 40
	ruis::key::unknown, // VK_SELECT
	ruis::key::unknown, // VK_PRINT
	ruis::key::unknown, // VK_EXECUTE
	ruis::key::print_screen, // VK_SNAPSHOT = print screen key
	ruis::key::insert, // VK_INSERT, 45
	ruis::key::deletion, // VK_DELETE
	ruis::key::unknown, // VK_HELP
	ruis::key::zero, // 0 key
	ruis::key::one, // 1 key
	ruis::key::two, // 2 key, 50
	ruis::key::three, // 3 key
	ruis::key::four, // 4 key
	ruis::key::five, // 5 key
	ruis::key::six, // 6 key
	ruis::key::seven, // 7 key, 55
	ruis::key::eight, // 8 key
	ruis::key::nine, // 9 key
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // Undefined, 60
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // Undefined
	ruis::key::unknown, // Undefined
	ruis::key::a, // A key, 65
	ruis::key::b, // B key
	ruis::key::c, // C key
	ruis::key::d, // D key
	ruis::key::e, // E key
	ruis::key::f, // F key, 70
	ruis::key::g, // G key
	ruis::key::h, // H key
	ruis::key::i, // I key
	ruis::key::j, // J key
	ruis::key::k, // K key, 75
	ruis::key::l, // L key
	ruis::key::m, // M key
	ruis::key::n, // N key
	ruis::key::o, // O key
	ruis::key::p, // P key, 80
	ruis::key::q, // Q key
	ruis::key::r, // R key
	ruis::key::s, // S key
	ruis::key::t, // T key
	ruis::key::u, // U key, 85
	ruis::key::v, // V key
	ruis::key::w, // W key
	ruis::key::x, // X key
	ruis::key::y, // Y key
	ruis::key::z, // Z key, 90
	ruis::key::left_command, // VK_LWIN = left windows key
	ruis::key::right_command, // VK_RWIN = right windows key
	ruis::key::unknown, // VK_APPS = applications key
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // VK_SLEEP = computer sleep key, 95
	ruis::key::zero, // VK_NUMPAD0 = numeric keypad 0 key
	ruis::key::one, // VK_NUMPAD1 = numeric keypad 1 key
	ruis::key::two, // VK_NUMPAD2 = numeric keypad 2 key
	ruis::key::three, // VK_NUMPAD3 = numeric keypad 3 key
	ruis::key::four, // VK_NUMPAD4 = numeric keypad 4 key, 100
	ruis::key::five, // VK_NUMPAD5 = numeric keypad 5 key
	ruis::key::six, // VK_NUMPAD6 = numeric keypad 6 key
	ruis::key::seven, // VK_NUMPAD7 = numeric keypad 7 key
	ruis::key::eight, // VK_NUMPAD8 = numeric keypad 8 key
	ruis::key::nine, // VK_NUMPAD9 = numeric keypad 9 key, 105
	ruis::key::unknown, // VK_MULTIPLY = multiply key
	ruis::key::unknown, // VK_ADD
	ruis::key::unknown, // VK_SEPARATOR
	ruis::key::unknown, // VK_SUBTRACT
	ruis::key::unknown, // VK_DECIMAL, 110
	ruis::key::unknown, // VK_DIVIDE
	ruis::key::f1, // VK_F1
	ruis::key::f2, // VK_F2
	ruis::key::f3, // VK_F3
	ruis::key::f4, // VK_F4, 115
	ruis::key::f5, // VK_F5
	ruis::key::f6, // VK_F6
	ruis::key::f7, // VK_F7
	ruis::key::f8, // VK_F8
	ruis::key::f9, // VK_F9, 120
	ruis::key::f10, // VK_F10
	ruis::key::f11, // VK_F11
	ruis::key::f12, // VK_F12
	ruis::key::unknown, // VK_F13
	ruis::key::unknown, // VK_F14, 125
	ruis::key::unknown, // VK_F15
	ruis::key::unknown, // VK_F16
	ruis::key::unknown, // VK_F17
	ruis::key::unknown, // VK_F18
	ruis::key::unknown, // VK_F19, 130
	ruis::key::unknown, // VK_F20
	ruis::key::unknown, // VK_F21
	ruis::key::unknown, // VK_F22
	ruis::key::unknown, // VK_F23
	ruis::key::unknown, // VK_F24, 135
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned, 140
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // VK_NUMLOCK
	ruis::key::unknown, // VK_SCROLL = scroll lock key, 145
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific, 150
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned, 155
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::left_shift, // VK_LSHIFT, 160
	ruis::key::right_shift, // VK_RSHIFT
	ruis::key::left_control, // VK_LCONTROL
	ruis::key::right_control, // VK_RCONTROL
	ruis::key::menu, // VK_LMENU = left menu key
	ruis::key::menu, // VK_RMENU, 165
	ruis::key::unknown, // VK_BROWSER_BACK
	ruis::key::unknown, // VK_BROWSER_FORWARD
	ruis::key::unknown, // VK_BROWSER_REFRESH
	ruis::key::unknown, // VK_BROWSER_STOP
	ruis::key::unknown, // VK_BROWSER_SEARCH, 170
	ruis::key::unknown, // VK_BROWSER_FAVORITES
	ruis::key::unknown, // VK_BROWSER_HOME
	ruis::key::unknown, // VK_VOLUME_MUTE
	ruis::key::unknown, // VK_VOLUME_DOWN
	ruis::key::unknown, // VK_VOLUME_UP, 175
	ruis::key::unknown, // VK_MEDIA_NEXT_TRACK
	ruis::key::unknown, // VK_MEDIA_PREV_TRACK
	ruis::key::unknown, // VK_MEDIA_STOP
	ruis::key::unknown, // VK_MEDIA_PLAY_PAUSE
	ruis::key::unknown, // VK_LAUNCH_MAIL, 180
	ruis::key::unknown, // VK_LAUNCH_MEDIA_SELECT
	ruis::key::unknown, // VK_LAUNCH_APP1
	ruis::key::unknown, // VK_LAUNCH_APP2
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved, 185
	ruis::key::unknown, // VK_OEM_1
	ruis::key::unknown, // VK_OEM_PLUS
	ruis::key::unknown, // VK_OEM_COMMA
	ruis::key::unknown, // VK_OEM_MINUS
	ruis::key::unknown, // VK_OEM_PERIOD, 190
	ruis::key::unknown, // VK_OEM_2
	ruis::key::unknown, // VK_OEM_3
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved, 195
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved, 200
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved, 205
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved, 210
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // Reserved, 215
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // VK_OEM_4
	ruis::key::unknown, // VK_OEM_5, 220
	ruis::key::unknown, // VK_OEM_6
	ruis::key::unknown, // VK_OEM_7
	ruis::key::unknown, // VK_OEM_8
	ruis::key::unknown, // Reserved
	ruis::key::unknown, // OEM specific, 225
	ruis::key::unknown, // VK_OEM_102
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // VK_PROCESSKEY
	ruis::key::unknown, // OEM specific, 230
	ruis::key::unknown, // VK_PACKET
	ruis::key::unknown, // Unassigned
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific, 235
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific, 240
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific
	ruis::key::unknown, // OEM specific, 245
	ruis::key::unknown, // VK_ATTN
	ruis::key::unknown, // VK_CRSEL
	ruis::key::unknown, // VK_EXSEL
	ruis::key::unknown, // VK_EREOF
	ruis::key::unknown, // VK_PLAY, 250
	ruis::key::unknown, // VK_ZOOM
	ruis::key::unknown, // VK_NONAME
	ruis::key::unknown, // VK_PA1
	ruis::key::unknown, // VK_OEM_CLEAR
	ruis::key::unknown
};

class windows_input_string_provider : public ruis::gui::input_string_provider
{
	char32_t c;

public:
	windows_input_string_provider(char32_t unicode_char = 0) :
		c(unicode_char)
	{}

	std::u32string get() const override
	{
		if (this->c == 0) {
			return {};
		}

		return {&this->c, 1};
	}
};
} // namespace

namespace {
LRESULT CALLBACK window_procedure(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg) {
		case WM_ACTIVATE:
			// TODO: do something on activate/deactivate?
			// if (!HIWORD(w_param)) { // Check Minimization State
			// 					   // window active
			// } else {
			// 	// window is no longer active
			// }
			return 0;

		case WM_SYSCOMMAND:
			switch (w_param) {
				case SC_SCREENSAVE: // screensaver trying to start?
				case SC_MONITORPOWER: // montor trying to enter powersave?
					return 0; // prevent from happening
				default:
					break;
			}
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;

		case WM_MOUSEMOVE:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				if (!ww.isHovered) {
					TRACKMOUSEEVENT tme = {sizeof(tme)};
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hwnd;
					TrackMouseEvent(&tme);

					ww.isHovered = true;

					// restore mouse cursor invisibility
					if (!ww.mouseCursorIsCurrentlyVisible) {
						CURSORINFO ci;
						ci.cbSize = sizeof(CURSORINFO);
						if (GetCursorInfo(&ci) != 0) {
							if (ci.flags & CURSOR_SHOWING) {
								ShowCursor(FALSE);
							}
						} else {
							utki::log_debug([&](auto& o) {
								o << "GetCursorInfo(): failed!!!" << std::endl;
							});
						}
					}

					handle_mouse_hover(ruisapp::inst(), true, 0);
				}
				handle_mouse_move(
					ruisapp::inst(),
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					0
				);
				return 0;
			}
		case WM_MOUSELEAVE:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));

				// Windows hides the mouse cursor even in non-client areas of the window,
				// like caption bar and borders, so show cursor if it is hidden
				if (!ww.mouseCursorIsCurrentlyVisible) {
					ShowCursor(TRUE);
				}

				ww.isHovered = false;
				handle_mouse_hover(ruisapp::inst(), false, 0);

				// Report mouse button up events for all pressed mouse buttons
				for (size_t i = 0; i != ww.mouseButtonState.size(); ++i) {
					auto btn = ruis::mouse_button(i);
					if (ww.mouseButtonState.get(btn)) {
						ww.mouseButtonState.clear(btn);
						constexpr auto outside_of_window_coordinate = 100000000;
						handle_mouse_button(
							ruisapp::inst(),
							false,
							ruis::vector2(outside_of_window_coordinate, outside_of_window_coordinate),
							btn,
							0
						);
					}
				}
				return 0;
			}
		case WM_LBUTTONDOWN:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.set(ruis::mouse_button::left);
				handle_mouse_button(
					ruisapp::inst(),
					true,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::left,
					0
				);
				return 0;
			}
		case WM_LBUTTONUP:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.clear(ruis::mouse_button::left);
				handle_mouse_button(
					ruisapp::inst(),
					false,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::left,
					0
				);
				return 0;
			}
		case WM_MBUTTONDOWN:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.set(ruis::mouse_button::middle);
				handle_mouse_button(
					ruisapp::inst(),
					true,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::middle,
					0
				);
				return 0;
			}
		case WM_MBUTTONUP:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.clear(ruis::mouse_button::middle);
				handle_mouse_button(
					ruisapp::inst(),
					false,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::middle,
					0
				);
				return 0;
			}
		case WM_RBUTTONDOWN:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.set(ruis::mouse_button::right);
				handle_mouse_button(
					ruisapp::inst(),
					true,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::right,
					0
				);
				return 0;
			}
		case WM_RBUTTONUP:
			{
				auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.clear(ruis::mouse_button::right);
				handle_mouse_button(
					ruisapp::inst(),
					false,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::right,
					0
				);
				return 0;
			}
		case WM_MOUSEWHEEL:
			[[fallthrough]];
		case WM_MOUSEHWHEEL:
			{
				unsigned short int times = HIWORD(w_param);
				times /= WHEEL_DELTA;
				ruis::mouse_button button = [&times, &msg]() {
					if (times >= 0) {
						return msg == WM_MOUSEWHEEL ? ruis::mouse_button::wheel_up : ruis::mouse_button::wheel_right;
					} else {
						times = -times;
						return msg == WM_MOUSEWHEEL ? ruis::mouse_button::wheel_down : ruis::mouse_button::wheel_left;
					}
				}();

				POINT pos;
				pos.x = GET_X_LPARAM(l_param);
				pos.y = GET_Y_LPARAM(l_param);

				// For some reason in WM_MOUSEWHEEL message mouse cursor position is sent in
				// screen coordinates, need to traslate those to window coordinates.
				if (ScreenToClient(hwnd, &pos) == 0) {
					break;
				}

				for (unsigned i = 0; i != times; ++i) {
					handle_mouse_button(ruisapp::inst(), true, ruis::vector2(float(pos.x), float(pos.y)), button, 0);
					handle_mouse_button(ruisapp::inst(), false, ruis::vector2(float(pos.x), float(pos.y)), button, 0);
				}
			}
			return 0;

		case WM_KEYDOWN:
			{
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				ruis::key key = key_code_map[uint8_t(w_param)];

				constexpr auto previous_key_state_mask = 0x40000000;

				if ((l_param & previous_key_state_mask) == 0) { // ignore auto-repeated keypress event
					handle_key_event(ruisapp::inst(), true, key);
				}
				handle_character_input(ruisapp::inst(), windows_input_string_provider(), key);
				return 0;
			}
		case WM_KEYUP:
			handle_key_event(
				ruisapp::inst(),
				false,
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				key_code_map[std::uint8_t(w_param)]
			);
			return 0;

		case WM_CHAR:
			switch (char32_t(w_param)) {
				case U'\U00000008': // Backspace character
				case U'\U0000001b': // Escape character
				case U'\U0000000d': // Carriage return
					break;
				default:
					handle_character_input(
						ruisapp::inst(),
						windows_input_string_provider(char32_t(w_param)),
						ruis::key::unknown
					);
					break;
			}
			return 0;
		case WM_PAINT:
			// we will redraw anyway on every cycle
			// app.Render();

			// Tell Windows that we have redrawn contents
			// and WM_PAINT message should go away from message queue.
			ValidateRect(hwnd, nullptr);
			return 0;

		case WM_SIZE:
			// LoWord=Width, HiWord=Height
			update_window_rect(
				ruisapp::inst(), //
				ruis::rect(0, 0, ruis::real(LOWORD(l_param)), ruis::real(HIWORD(l_param)))
			);
			return 0;

		case WM_USER:
			{
				std::unique_ptr<std::function<void()>> m(
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
					reinterpret_cast<std::function<void()>*>(l_param)
				);
				(*m)();
			}
			return 0;

		default:
			break;
	}

	return DefWindowProc(hwnd, msg, w_param, l_param);
}
} // namespace

namespace {
ruis::real get_dots_per_inch(HDC dc)
{
	constexpr auto num_dimensions = 2;
	// average dots per cm over device dimensions
	ruis::real dots_per_cm =
		(ruis::real(GetDeviceCaps(dc, HORZRES)) * std::deci::den / ruis::real(GetDeviceCaps(dc, HORZSIZE)) +
		 ruis::real(GetDeviceCaps(dc, VERTRES)) * std::deci::den / ruis::real(GetDeviceCaps(dc, VERTSIZE))) /
		ruis::real(num_dimensions);

	return ruis::real(dots_per_cm) * ruis::real(utki::cm_per_inch);
}
} // namespace

namespace {
ruis::real get_dots_per_pp(HDC dc)
{
	r4::vector2<unsigned> resolution(GetDeviceCaps(dc, HORZRES), GetDeviceCaps(dc, VERTRES));
	r4::vector2<unsigned> screen_size_mm(GetDeviceCaps(dc, HORZSIZE), GetDeviceCaps(dc, VERTSIZE));

	return ruisapp::application::get_pixels_per_pp(resolution, screen_size_mm);
}
} // namespace

namespace {
ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	// the variable is initialized via output argument, so no need
	// to initialize it here
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	std::array<CHAR, MAX_PATH> path;
	if (SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, path.data()) != S_OK) {
		throw std::runtime_error("failed to get user's profile directory.");
	}

	path.back() = '\0'; // null-terminate the string just in case

	std::string home_dir(path.data(), strlen(path.data()));
	ASSERT(!home_dir.empty())

	std::replace(
		home_dir.begin(), //
		home_dir.end(),
		'\\',
		'/'
	);

	home_dir = papki::as_dir(home_dir);

	home_dir.append(1, '.').append(app_name).append(1, '/');

	ruisapp::application::directories dirs;

	dirs.cache = utki::cat(home_dir, "cache/");
	dirs.config = utki::cat(home_dir, "config/");
	dirs.state = utki::cat(home_dir, "state/");

	return dirs;
}
} // namespace

application::application(
	std::string name, //
	const window_parameters& wp
) :
	name(std::move(name)),
	window_pimpl(std::make_unique<window_wrapper>(wp)),
	gui(utki::make_shared<ruis::context>(
		utki::make_shared<ruis::style_provider>( //
			utki::make_shared<ruis::resource_loader>( //
				utki::make_shared<ruis::render::renderer>( //
#ifdef RUISAPP_RENDER_OPENGL
					utki::make_shared<ruis::render::opengl::context>()
#elif defined(RUISAPP_RENDER_OPENGLES)
					utki::make_shared<ruis::render::opengles::context>()
#else
#	error "Unknown graphics API"
#endif
				)
			)
		),
		utki::make_shared<ruis::updater>(),
		ruis::context::parameters{
			.post_to_ui_thread_function =
				[](std::function<void()> procedure) {
					auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
					if (PostMessage(
							ww.hwnd,
							WM_USER,
							0,
							// NOLINTNEXTLINE(cppcoreguidelines-owning-memory,
							// cppcoreguidelines-pro-type-reinterpret-cast)
							reinterpret_cast<LPARAM>(
								new std::remove_reference_t<decltype(procedure)>(std::move(procedure))
							)
						) == 0)
					{
						throw std::runtime_error("PostMessage(): failed");
					}
				},
			.set_mouse_cursor_function =
				[](ruis::mouse_cursor c) {
					// TODO:
				},
			.units = ruis::units(
				get_dots_per_inch(get_impl(this->window_pimpl).hdc), //
				get_dots_per_pp(get_impl(this->window_pimpl).hdc)
			)
		}
	)),
	directory(get_application_directories(this->name)),
	cur_window_rect(0, 0, -1, -1)
{
	this->update_window_rect(ruis::rect(0, 0, ruis::real(wp.dims.x()), ruis::real(wp.dims.y())));
}

void application::quit() noexcept
{
	auto& ww = get_impl(this->window_pimpl);
	ww.quitFlag = true;
}

namespace ruisapp {
void winmain(
	int argc, //
	const char** argv
)
{
	auto app = ruisapp::application_factory::make_application(argc, argv);
	if (!app) {
		// Not an error. The application just did not show any GUI to the user and exited normally.
		return;
	}

	ASSERT(app)

	auto& ww = get_impl(get_window_pimpl(*app));

	ShowWindow(ww.hwnd, SW_SHOW);

	while (!ww.quitFlag) {
		// sequence:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle
		uint32_t timeout = app->gui.update();
		render(*app);
		DWORD status = MsgWaitForMultipleObjectsEx(0, nullptr, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

		//		TRACE(<< "msg" << std::endl)

		if (status == WAIT_OBJECT_0) {
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				//				TRACE(<< "msg got, msg.message = " <<
				// msg.message << std::endl)
				if (msg.message == WM_QUIT) {
					ww.quitFlag = true;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		//		TRACE(<< "loop" << std::endl)
	}
}
} // namespace ruisapp

int WINAPI WinMain(
	HINSTANCE h_instance, // Instance
	HINSTANCE h_prev_instance, // Previous Instance
	LPSTR lp_cmd_line, // Command Line Parameters
	int n_cmd_show // Window Show State
)
{
	ruisapp::winmain(__argc, const_cast<const char**>(__argv));

	return 0;
}

int main(int argc, const char** argv)
{
	ruisapp::winmain(argc, argv);

	return 0;
}

void application::set_fullscreen(bool enable)
{
	if (enable == this->is_fullscreen()) {
		return;
	}

	auto& ww = get_impl(this->window_pimpl);

	if (enable) {
		// save original window size
		RECT rect;
		if (GetWindowRect(ww.hwnd, &rect) == 0) {
			throw std::runtime_error("Failed to get window rect");
		}
		this->before_fullscreen_window_rect.p.x() = rect.left;
		this->before_fullscreen_window_rect.p.y() = rect.top;
		this->before_fullscreen_window_rect.d.x() = rect.right - rect.left;
		this->before_fullscreen_window_rect.d.y() = rect.bottom - rect.top;

		// Set new window style
		SetWindowLong(ww.hwnd, GWL_STYLE, GetWindowLong(ww.hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME));
		SetWindowLong(
			ww.hwnd,
			GWL_EXSTYLE,
			GetWindowLong(ww.hwnd, GWL_EXSTYLE) &
				~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
		);

		// set new window size and position
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(MonitorFromWindow(ww.hwnd, MONITOR_DEFAULTTONEAREST), &mi);
		SetWindowPos(
			ww.hwnd,
			nullptr,
			mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
		);
	} else {
		// Reset original window style
		SetWindowLong(ww.hwnd, GWL_STYLE, GetWindowLong(ww.hwnd, GWL_STYLE) | (WS_CAPTION | WS_THICKFRAME));
		SetWindowLong(
			ww.hwnd,
			GWL_EXSTYLE,
			GetWindowLong(ww.hwnd, GWL_EXSTYLE) |
				(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
		);

		SetWindowPos(
			ww.hwnd,
			nullptr,
			this->before_fullscreen_window_rect.p.x(),
			this->before_fullscreen_window_rect.p.y(),
			this->before_fullscreen_window_rect.d.x(),
			this->before_fullscreen_window_rect.d.y(),
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
		);
	}

	this->is_fullscreen_v = enable;
}

void application::set_mouse_cursor_visible(bool visible)
{
	auto& ww = get_impl(this->window_pimpl);

	if (visible) {
		if (!ww.mouseCursorIsCurrentlyVisible) {
			ShowCursor(TRUE);
			ww.mouseCursorIsCurrentlyVisible = true;
		}
	} else {
		if (ww.mouseCursorIsCurrentlyVisible) {
			ShowCursor(FALSE);
			ww.mouseCursorIsCurrentlyVisible = false;
		}
	}
}

void application::swap_frame_buffers()
{
	auto& ww = get_impl(this->window_pimpl);
#ifdef RUISAPP_RENDER_OPENGL
	SwapBuffers(ww.hdc);
#elif defined(RUISAPP_RENDER_OPENGLES)
	eglSwapBuffers(ww.egl_display, ww.egl_surface);
#else
#	error "Unknown graphics API"
#endif
}

namespace {
window_wrapper::window_wrapper(const window_parameters& wp)
{
	{
		WNDCLASS wc;
		memset(&wc, 0, sizeof(wc));

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // redraw on resize, own DC for window
		wc.lpfnWndProc = (WNDPROC)window_procedure;
		wc.cbClsExtra = 0; // no extra window data
		wc.cbWndExtra = 0; // no extra window data
		wc.hInstance = GetModuleHandle(nullptr); // instance handle
		wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr; // no background required for OpenGL
		wc.lpszMenuName = nullptr; // we don't want a menu
		wc.lpszClassName = window_class_name; // set the window class Name

		if (!RegisterClass(&wc)) {
			throw std::runtime_error("Failed to register window class");
		}
	}

	utki::scope_exit scope_exit_window_class([]() {
		if (!UnregisterClass(window_class_name, GetModuleHandle(nullptr))) {
			ASSERT(false, [&](auto& o) {
				o << "Failed to unregister window class";
			})
		}
	});

	// we need to register window class name before creating the window,
	// this is why we dont initialize hwnd in the constructor initializer list
	// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
	this->hwnd = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, // extended style
		window_class_name,
		wp.title.c_str(),
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, // x
		0, // y
		int(wp.dims.x()) + 2 * GetSystemMetrics(SM_CXSIZEFRAME),
		int(wp.dims.y()) + GetSystemMetrics(SM_CYCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME),
		nullptr, // no parent window
		nullptr, // no menu
		GetModuleHandle(nullptr),
		nullptr // do not pass anything to WM_CREATE
	);

	if (!this->hwnd) {
		throw std::runtime_error("Failed to create a window");
	}

	utki::scope_exit scope_exit_hwnd([this]() {
		if (!DestroyWindow(this->hwnd)) {
			ASSERT(false, [&](auto& o) {
				o << "Failed to destroy window";
			})
		}
	});

	// NOTE: window will be shown later, right before entering main loop and after
	// all initial App data is initialized

	this->hdc = GetDC(this->hwnd);
	if (!this->hdc) {
		throw std::runtime_error("Failed to create a OpenGL device context");
	}

	utki::scope_exit scope_exit_hdc([this]() {
		if (!ReleaseDC(this->hwnd, this->hdc)) {
			ASSERT(false, [&](auto& o) {
				o << "Failed to release device context";
			})
		}
	});

#ifdef RUISAPP_RENDER_OPENGL
	{
		static PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR),
			1, // Version number of the structure, should be 1
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			BYTE(PFD_TYPE_RGBA),
			BYTE(utki::byte_bits * 4), // 32 bit color depth
			BYTE(0),
			BYTE(0),
			BYTE(0),
			BYTE(0),
			BYTE(0),
			BYTE(0), // color bits ignored
			BYTE(0), // no alpha buffer
			BYTE(0), // shift bit ignored
			BYTE(0), // no accumulation buffer
			BYTE(0),
			BYTE(0),
			BYTE(0),
			BYTE(0), // accumulation bits ignored
			wp.buffers.get(ruisapp::buffer::depth) ? BYTE(utki::byte_bits * 2) : BYTE(0), // 16 bit depth buffer
			wp.buffers.get(ruisapp::buffer::stencil) ? BYTE(utki::byte_bits) : BYTE(0),
			BYTE(0), // no auxiliary buffer
			BYTE(PFD_MAIN_PLANE), // main drawing layer
			BYTE(0), // reserved
			0,
			0,
			0 // layer masks ignored
		};

		int pixel_format = ChoosePixelFormat(this->hdc, &pfd);
		if (!pixel_format) {
			throw std::runtime_error("Could not find suitable pixel format");
		}

		//	TRACE_AND_LOG(<<
		//"application::DeviceContextWrapper::DeviceContextWrapper(): pixel format
		// chosen" <<
		// std::endl)

		if (!SetPixelFormat(this->hdc, pixel_format, &pfd)) {
			throw std::runtime_error("Could not sent pixel format");
		}
	}

	this->hrc = wglCreateContext(hdc);
	if (!this->hrc) {
		throw std::runtime_error("Failed to create OpenGL rendering context");
	}

	utki::scope_exit scope_exit_hrc([this]() {
		if (!wglMakeCurrent(nullptr, nullptr)) {
			ASSERT(false, [&](auto& o) {
				o << "Deactivating OpenGL rendering context failed";
			})
		}
		if (!wglDeleteContext(this->hrc)) {
			ASSERT(false, [&](auto& o) {
				o << "Releasing OpenGL rendering context failed";
			})
		}
	});

	//	TRACE_AND_LOG(<< "application::GLContextWrapper::GLContextWrapper(): GL
	// rendering context created" << std::endl)

	if (!wglMakeCurrent(hdc, this->hrc)) {
		throw std::runtime_error("Failed to activate OpenGL rendering context");
	}

	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("GLEW initialization failed");
	}

	scope_exit_hrc.release();

#elif defined(RUISAPP_RENDER_OPENGLES)

	auto graphics_api_version = [&ver = wp.graphics_api_version]() {
		if (ver.to_uint32_t() == 0) {
			// default OpenGL ES version is 2.0
			return utki::version_duplet{
				.major = 2, //
				.minor = 0
			};
		}
		return ver;
	}();

	this->egl_display = eglGetDisplay(this->hdc);
	if (this->egl_display == EGL_NO_DISPLAY) {
		throw std::runtime_error("Failed to get EGL display");
	}

	if (!eglInitialize(this->egl_display, nullptr, nullptr)) {
		throw std::runtime_error("Failed to initialize EGL");
	}

	utki::scope_exit scope_exit_display([this]() {
		if (!eglTerminate(this->egl_display)) {
			ASSERT(false, [&](auto& o) {
				o << "Terminating EGL failed";
			})
		}
	});

	EGLConfig config = nullptr;
	{
		const std::array<EGLint, 15> config_attribs = {
			EGL_SURFACE_TYPE,
			EGL_WINDOW_BIT,
			EGL_RENDERABLE_TYPE,
			// We cannot set bits for all OpenGL ES versions because on platforms which do not
			// support later versions the matching config will not be found by eglChooseConfig().
			// So, set bits according to requested OpenGL ES version.
			[&ver = wp.graphics_api_version]() {
				EGLint ret = EGL_OPENGL_ES2_BIT; // OpenGL ES 2 is the minimum
				if (ver.major >= 3) {
					ret |= EGL_OPENGL_ES3_BIT;
				}
				return ret;
			}(),
			EGL_RED_SIZE,
			8,
			EGL_GREEN_SIZE,
			8,
			EGL_BLUE_SIZE,
			8,
			EGL_DEPTH_SIZE,
			wp.buffers.get(ruisapp::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
			EGL_STENCIL_SIZE,
			wp.buffers.get(ruisapp::buffer::stencil) ? utki::byte_bits : 0,
			EGL_NONE
		};

		EGLint num_configs = 0;
		if (!eglChooseConfig(this->egl_display, config_attribs.data(), &config, 1, &num_configs)) {
			throw std::runtime_error("Failed to choose EGL config");
		}
	}

	this->egl_surface = eglCreateWindowSurface(
		this->egl_display, //
		config,
		EGLNativeWindowType(this->hwnd),
		nullptr
	);
	if (this->egl_surface == EGL_NO_SURFACE) {
		throw std::runtime_error("Failed to create EGL surface");
	}

	utki::scope_exit scope_exit_surface([this]() {
		if (!eglDestroySurface(this->egl_display, this->egl_surface)) {
			ASSERT(false, [&](auto& o) {
				o << "Destroying EGL surface failed";
			})
		}
	});

	{
		constexpr auto attrs_array_size = 5;
		std::array<EGLint, attrs_array_size> context_attrs = {
			EGL_CONTEXT_MAJOR_VERSION,
			graphics_api_version.major,
			EGL_CONTEXT_MINOR_VERSION,
			graphics_api_version.minor,
			EGL_NONE
		};

		this->egl_context = eglCreateContext(this->egl_display, config, EGL_NO_CONTEXT, context_attrs.data());
		if (this->egl_context == EGL_NO_CONTEXT) {
			throw std::runtime_error("Failed to create EGL context");
		}
	}

	utki::scope_exit scope_exit_context([this]() {
		if (!eglDestroyContext(this->egl_display, this->egl_context)) {
			ASSERT(false, [&](auto& o) {
				o << "Destroying EGL context failed";
			})
		}
	});

	if (!eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context)) {
		throw std::runtime_error("Failed to make EGL context current");
	}

	scope_exit_context.release();
	scope_exit_surface.release();
	scope_exit_display.release();
#else
#	error "Unknown graphics API"
#endif

	scope_exit_hdc.release();
	scope_exit_hwnd.release();
	scope_exit_window_class.release();
}

window_wrapper::~window_wrapper()
{
#ifdef RUISAPP_RENDER_OPENGL

	if (!wglMakeCurrent(nullptr, nullptr)) {
		ASSERT(false, [&](auto& o) {
			o << "Deactivating OpenGL rendering context failed";
		})
	}

	if (!wglDeleteContext(this->hrc)) {
		ASSERT(false, [&](auto& o) {
			o << "Releasing OpenGL rendering context failed";
		})
	}

#elif defined(RUISAPP_RENDER_OPENGLES)

	if (!eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
		ASSERT(false, [&](auto& o) {
			o << "Deactivating EGL context failed";
		})
	}

	if (!eglDestroyContext(this->egl_display, this->egl_context)) {
		ASSERT(false, [&](auto& o) {
			o << "Destroying EGL context failed";
		})
	}

	if (!eglDestroySurface(this->egl_display, this->egl_surface)) {
		ASSERT(false, [&](auto& o) {
			o << "Destroying EGL surface failed";
		})
	}

	if (!eglTerminate(this->egl_display)) {
		ASSERT(false, [&](auto& o) {
			o << "Terminating EGL failed";
		})
	}

#else
#	error "Unknown graphics API"
#endif

	if (!ReleaseDC(this->hwnd, this->hdc)) {
		ASSERT(false, [&](auto& o) {
			o << "Failed to release device context";
		})
	}
	if (!DestroyWindow(this->hwnd)) {
		ASSERT(false, [&](auto& o) {
			o << "Failed to destroy window";
		})
	}
	if (!UnregisterClass(window_class_name, GetModuleHandle(nullptr))) {
		ASSERT(false, [&](auto& o) {
			o << "Failed to unregister window class";
		})
	}
}
} // namespace
