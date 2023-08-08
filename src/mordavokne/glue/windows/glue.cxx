/*
mordavokne - morda GUI adaptation layer

Copyright (C) 2016-2021  Ivan Gagis <igagis@gmail.com>

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
#include <morda/context.hpp>
#include <morda/render/opengl/renderer.hpp>
#include <morda/util/util.hpp>
#include <papki/fs_file.hpp>
#include <utki/destructable.hpp>
#include <utki/windows.hpp>
#include <windowsx.h> // needed for GET_X_LPARAM macro and other similar macros

#include "../../application.hpp"

// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "../friend_accessors.cxx"

using namespace mordavokne;

namespace {
constexpr const char* window_class_name = "MordavokneWindowClassName";
} // namespace

namespace {
struct window_wrapper : public utki::destructable {
	HWND hwnd;
	HDC hdc;
	HGLRC hrc;

	bool quitFlag = false;

	bool isHovered = false; // for tracking when mouse enters or leaves window.

	utki::flags<morda::mouse_button> mouseButtonState;

	bool mouseCursorIsCurrentlyVisible = true;

	window_wrapper(const window_params& wp);

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
const std::array<morda::key, std::numeric_limits<uint8_t>::max() + 1> key_code_map = {
	morda::key::unknown, // Undefined
	morda::key::unknown, // VK_LBUTTON
	morda::key::unknown, // VK_RBUTTON
	morda::key::unknown, // VK_CANCEL
	morda::key::unknown, // VK_MBUTTON
	morda::key::unknown, // VK_XBUTTON1, 5
	morda::key::unknown, // VK_XBUTTON2
	morda::key::unknown, // Undefined
	morda::key::backspace, // VK_BACK = backspace key
	morda::key::tabulator, // VK_TAB
	morda::key::unknown, // Reserved, 10
	morda::key::unknown, // Reserved
	morda::key::unknown, // VK_CLEAR = clear key???
	morda::key::enter, // VK_RETURN
	morda::key::unknown, // Undefined
	morda::key::unknown, // Undefined, 15
	morda::key::left_shift, // VK_SHIFT
	morda::key::left_control, // VK_CONTROL
	morda::key::left_alt, // VK_MENU = alt key
	morda::key::pause, // VK_PAUSE
	morda::key::capslock, // VK_CAPITAL = caps lock key, 20
	morda::key::unknown, // VK_KANA, VK_HANGUEL, VK_HANGUL
	morda::key::unknown, // Undefined
	morda::key::unknown, // VK_JUNJA
	morda::key::unknown, // VK_FINAL
	morda::key::unknown, // VK_HANJA, VK_KANJI, 25
	morda::key::unknown, // Undefined
	morda::key::escape, // VK_ESCAPE
	morda::key::unknown, // VK_CONVERT
	morda::key::unknown, // VK_NONCONVERT
	morda::key::unknown, // VK_ACCEPT, 30
	morda::key::unknown, // VK_MODECHANGE
	morda::key::space, // VK_SPACE = space bar key
	morda::key::page_up, // VK_PRIOR = page up key
	morda::key::page_down, // VK_NEXT = page down key
	morda::key::end, // VK_END, 35
	morda::key::home, // VK_HOME
	morda::key::arrow_left, // VK_LEFT
	morda::key::arrow_up, // VK_UP
	morda::key::arrow_right, // VK_RIGHT
	morda::key::arrow_down, // VK_DOWN, 40
	morda::key::unknown, // VK_SELECT
	morda::key::unknown, // VK_PRINT
	morda::key::unknown, // VK_EXECUTE
	morda::key::print_screen, // VK_SNAPSHOT = print screen key
	morda::key::insert, // VK_INSERT, 45
	morda::key::deletion, // VK_DELETE
	morda::key::unknown, // VK_HELP
	morda::key::zero, // 0 key
	morda::key::one, // 1 key
	morda::key::two, // 2 key, 50
	morda::key::three, // 3 key
	morda::key::four, // 4 key
	morda::key::five, // 5 key
	morda::key::six, // 6 key
	morda::key::seven, // 7 key, 55
	morda::key::eight, // 8 key
	morda::key::nine, // 9 key
	morda::key::unknown, // Undefined
	morda::key::unknown, // Undefined
	morda::key::unknown, // Undefined, 60
	morda::key::unknown, // Undefined
	morda::key::unknown, // Undefined
	morda::key::unknown, // Undefined
	morda::key::unknown, // Undefined
	morda::key::a, // A key, 65
	morda::key::b, // B key
	morda::key::c, // C key
	morda::key::d, // D key
	morda::key::e, // E key
	morda::key::f, // F key, 70
	morda::key::g, // G key
	morda::key::h, // H key
	morda::key::i, // I key
	morda::key::j, // J key
	morda::key::k, // K key, 75
	morda::key::l, // L key
	morda::key::m, // M key
	morda::key::n, // N key
	morda::key::o, // O key
	morda::key::p, // P key, 80
	morda::key::q, // Q key
	morda::key::r, // R key
	morda::key::s, // S key
	morda::key::t, // T key
	morda::key::u, // U key, 85
	morda::key::v, // V key
	morda::key::w, // W key
	morda::key::x, // X key
	morda::key::y, // Y key
	morda::key::z, // Z key, 90
	morda::key::left_command, // VK_LWIN = left windows key
	morda::key::right_command, // VK_RWIN = right windows key
	morda::key::unknown, // VK_APPS = applications key
	morda::key::unknown, // Reserved
	morda::key::unknown, // VK_SLEEP = computer sleep key, 95
	morda::key::zero, // VK_NUMPAD0 = numeric keypad 0 key
	morda::key::one, // VK_NUMPAD1 = numeric keypad 1 key
	morda::key::two, // VK_NUMPAD2 = numeric keypad 2 key
	morda::key::three, // VK_NUMPAD3 = numeric keypad 3 key
	morda::key::four, // VK_NUMPAD4 = numeric keypad 4 key, 100
	morda::key::five, // VK_NUMPAD5 = numeric keypad 5 key
	morda::key::six, // VK_NUMPAD6 = numeric keypad 6 key
	morda::key::seven, // VK_NUMPAD7 = numeric keypad 7 key
	morda::key::eight, // VK_NUMPAD8 = numeric keypad 8 key
	morda::key::nine, // VK_NUMPAD9 = numeric keypad 9 key, 105
	morda::key::unknown, // VK_MULTIPLY = multiply key
	morda::key::unknown, // VK_ADD
	morda::key::unknown, // VK_SEPARATOR
	morda::key::unknown, // VK_SUBTRACT
	morda::key::unknown, // VK_DECIMAL, 110
	morda::key::unknown, // VK_DIVIDE
	morda::key::f1, // VK_F1
	morda::key::f2, // VK_F2
	morda::key::f3, // VK_F3
	morda::key::f4, // VK_F4, 115
	morda::key::f5, // VK_F5
	morda::key::f6, // VK_F6
	morda::key::f7, // VK_F7
	morda::key::f8, // VK_F8
	morda::key::f9, // VK_F9, 120
	morda::key::f10, // VK_F10
	morda::key::f11, // VK_F11
	morda::key::f12, // VK_F12
	morda::key::unknown, // VK_F13
	morda::key::unknown, // VK_F14, 125
	morda::key::unknown, // VK_F15
	morda::key::unknown, // VK_F16
	morda::key::unknown, // VK_F17
	morda::key::unknown, // VK_F18
	morda::key::unknown, // VK_F19, 130
	morda::key::unknown, // VK_F20
	morda::key::unknown, // VK_F21
	morda::key::unknown, // VK_F22
	morda::key::unknown, // VK_F23
	morda::key::unknown, // VK_F24, 135
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned, 140
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // VK_NUMLOCK
	morda::key::unknown, // VK_SCROLL = scroll lock key, 145
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific, 150
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned, 155
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::left_shift, // VK_LSHIFT, 160
	morda::key::right_shift, // VK_RSHIFT
	morda::key::left_control, // VK_LCONTROL
	morda::key::right_control, // VK_RCONTROL
	morda::key::menu, // VK_LMENU = left menu key
	morda::key::menu, // VK_RMENU, 165
	morda::key::unknown, // VK_BROWSER_BACK
	morda::key::unknown, // VK_BROWSER_FORWARD
	morda::key::unknown, // VK_BROWSER_REFRESH
	morda::key::unknown, // VK_BROWSER_STOP
	morda::key::unknown, // VK_BROWSER_SEARCH, 170
	morda::key::unknown, // VK_BROWSER_FAVORITES
	morda::key::unknown, // VK_BROWSER_HOME
	morda::key::unknown, // VK_VOLUME_MUTE
	morda::key::unknown, // VK_VOLUME_DOWN
	morda::key::unknown, // VK_VOLUME_UP, 175
	morda::key::unknown, // VK_MEDIA_NEXT_TRACK
	morda::key::unknown, // VK_MEDIA_PREV_TRACK
	morda::key::unknown, // VK_MEDIA_STOP
	morda::key::unknown, // VK_MEDIA_PLAY_PAUSE
	morda::key::unknown, // VK_LAUNCH_MAIL, 180
	morda::key::unknown, // VK_LAUNCH_MEDIA_SELECT
	morda::key::unknown, // VK_LAUNCH_APP1
	morda::key::unknown, // VK_LAUNCH_APP2
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved, 185
	morda::key::unknown, // VK_OEM_1
	morda::key::unknown, // VK_OEM_PLUS
	morda::key::unknown, // VK_OEM_COMMA
	morda::key::unknown, // VK_OEM_MINUS
	morda::key::unknown, // VK_OEM_PERIOD, 190
	morda::key::unknown, // VK_OEM_2
	morda::key::unknown, // VK_OEM_3
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved, 195
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved, 200
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved, 205
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved, 210
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved
	morda::key::unknown, // Reserved, 215
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // Unassigned
	morda::key::unknown, // VK_OEM_4
	morda::key::unknown, // VK_OEM_5, 220
	morda::key::unknown, // VK_OEM_6
	morda::key::unknown, // VK_OEM_7
	morda::key::unknown, // VK_OEM_8
	morda::key::unknown, // Reserved
	morda::key::unknown, // OEM specific, 225
	morda::key::unknown, // VK_OEM_102
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // VK_PROCESSKEY
	morda::key::unknown, // OEM specific, 230
	morda::key::unknown, // VK_PACKET
	morda::key::unknown, // Unassigned
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific, 235
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific, 240
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific
	morda::key::unknown, // OEM specific, 245
	morda::key::unknown, // VK_ATTN
	morda::key::unknown, // VK_CRSEL
	morda::key::unknown, // VK_EXSEL
	morda::key::unknown, // VK_EREOF
	morda::key::unknown, // VK_PLAY, 250
	morda::key::unknown, // VK_ZOOM
	morda::key::unknown, // VK_NONAME
	morda::key::unknown, // VK_PA1
	morda::key::unknown, // VK_OEM_CLEAR
	morda::key::unknown
};

class windows_input_string_provider : public morda::gui::input_string_provider
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
			}
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;

		case WM_MOUSEMOVE:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
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
							LOG([&](auto& o) {
								o << "GetCursorInfo(): failed!!!" << std::endl;
							})
						}
					}

					handle_mouse_hover(mordavokne::inst(), true, 0);
				}
				handle_mouse_move(
					mordavokne::inst(),
					morda::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					0
				);
				return 0;
			}
		case WM_MOUSELEAVE:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));

				// Windows hides the mouse cursor even in non-client areas of the window,
				// like caption bar and borders, so show cursor if it is hidden
				if (!ww.mouseCursorIsCurrentlyVisible) {
					ShowCursor(TRUE);
				}

				ww.isHovered = false;
				handle_mouse_hover(mordavokne::inst(), false, 0);

				// Report mouse button up events for all pressed mouse buttons
				for (size_t i = 0; i != ww.mouseButtonState.size(); ++i) {
					auto btn = morda::mouse_button(i);
					if (ww.mouseButtonState.get(btn)) {
						ww.mouseButtonState.clear(btn);
						constexpr auto outside_of_window_coordinate = 100000000;
						handle_mouse_button(
							mordavokne::inst(),
							false,
							morda::vector2(outside_of_window_coordinate, outside_of_window_coordinate),
							btn,
							0
						);
					}
				}
				return 0;
			}
		case WM_LBUTTONDOWN:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
				ww.mouseButtonState.set(morda::mouse_button::left);
				handle_mouse_button(
					mordavokne::inst(),
					true,
					morda::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					morda::mouse_button::left,
					0
				);
				return 0;
			}
		case WM_LBUTTONUP:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
				ww.mouseButtonState.clear(morda::mouse_button::left);
				handle_mouse_button(
					mordavokne::inst(),
					false,
					morda::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					morda::mouse_button::left,
					0
				);
				return 0;
			}
		case WM_MBUTTONDOWN:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
				ww.mouseButtonState.set(morda::mouse_button::middle);
				handle_mouse_button(
					mordavokne::inst(),
					true,
					morda::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					morda::mouse_button::middle,
					0
				);
				return 0;
			}
		case WM_MBUTTONUP:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
				ww.mouseButtonState.clear(morda::mouse_button::middle);
				handle_mouse_button(
					mordavokne::inst(),
					false,
					morda::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					morda::mouse_button::middle,
					0
				);
				return 0;
			}
		case WM_RBUTTONDOWN:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
				ww.mouseButtonState.set(morda::mouse_button::right);
				handle_mouse_button(
					mordavokne::inst(),
					true,
					morda::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					morda::mouse_button::right,
					0
				);
				return 0;
			}
		case WM_RBUTTONUP:
			{
				auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
				ww.mouseButtonState.clear(morda::mouse_button::right);
				handle_mouse_button(
					mordavokne::inst(),
					false,
					morda::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					morda::mouse_button::right,
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
				morda::mouse_button button = [&times, &msg]() {
					if (times >= 0) {
						return msg == WM_MOUSEWHEEL ? morda::mouse_button::wheel_up : morda::mouse_button::wheel_right;
					} else {
						times = -times;
						return msg == WM_MOUSEWHEEL ? morda::mouse_button::wheel_down : morda::mouse_button::wheel_left;
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
					handle_mouse_button(
						mordavokne::inst(),
						true,
						morda::vector2(float(pos.x), float(pos.y)),
						button,
						0
					);
					handle_mouse_button(
						mordavokne::inst(),
						false,
						morda::vector2(float(pos.x), float(pos.y)),
						button,
						0
					);
				}
			}
			return 0;

		case WM_KEYDOWN:
			{
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				morda::key key = key_code_map[uint8_t(w_param)];

				constexpr auto previous_key_state_mask = 0x40000000;

				if ((l_param & previous_key_state_mask) == 0) { // ignore auto-repeated keypress event
					handle_key_event(mordavokne::inst(), true, key);
				}
				handle_character_input(mordavokne::inst(), windows_input_string_provider(), key);
				return 0;
			}
		case WM_KEYUP:
			handle_key_event(
				mordavokne::inst(),
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
						mordavokne::inst(),
						windows_input_string_provider(char32_t(w_param)),
						morda::key::unknown
					);
					break;
			}
			return 0;
		case WM_PAINT:
			// we will redraw anyway on every cycle
			// app.Render();
			ValidateRect(
				hwnd,
				nullptr
			); // This is to tell Windows that we have redrawn contents
			   // and WM_PAINT should go away from message queue.
			return 0;

		case WM_SIZE:
			// resize GL, LoWord=Width, HiWord=Height
			update_window_rect(
				mordavokne::inst(),
				morda::rectangle(0, 0, float(LOWORD(l_param)), float(HIWORD(l_param)))
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
morda::real get_dots_per_inch(HDC dc)
{
	constexpr auto num_dimensions = 2;
	// average dots per cm over device dimensions
	morda::real dots_per_cm =
		(morda::real(GetDeviceCaps(dc, HORZRES)) * std::deci::den / morda::real(GetDeviceCaps(dc, HORZSIZE))
		 + morda::real(GetDeviceCaps(dc, VERTRES)) * std::deci::den / morda::real(GetDeviceCaps(dc, VERTSIZE)))
		/ morda::real(num_dimensions);

	constexpr auto cm_per_inch = 2.54;

	return morda::real(dots_per_cm) * cm_per_inch;
}
} // namespace

namespace {
morda::real get_dots_per_pp(HDC dc)
{
	r4::vector2<unsigned> resolution(GetDeviceCaps(dc, HORZRES), GetDeviceCaps(dc, VERTRES));
	r4::vector2<unsigned> screen_size_mm(GetDeviceCaps(dc, HORZSIZE), GetDeviceCaps(dc, VERTSIZE));

	return mordavokne::application::get_pixels_per_dp(resolution, screen_size_mm);
}
} // namespace

namespace {
std::string initialize_storage_dir(const std::string& app_name)
{
	// the variable is initialized via output argument, so no need
	// to initialize it here
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	std::array<CHAR, MAX_PATH> path;
	if (SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, path.data()) != S_OK) {
		throw std::runtime_error("failed to get user's profile directory.");
	}

	path.back() = '\0'; // null-terminate the string just in case

	std::string home_dir_str(path.data(), strlen(path.data()));

	ASSERT(home_dir_str.size() != 0)

	if (home_dir_str[home_dir_str.size() - 1] == '\\') {
		home_dir_str[home_dir_str.size() - 1] = '/';
	}

	if (home_dir_str[home_dir_str.size() - 1] != '/') {
		home_dir_str.append(1, '/');
	}

	home_dir_str.append(1, '.').append(app_name).append(1, '/');

	papki::fs_file dir(home_dir_str);
	if (!dir.exists()) {
		dir.make_dir();
	}

	return home_dir_str;
}
} // namespace

application::application(std::string name, const window_params& wp) :
	name(std::move(name)),
	window_pimpl(std::make_unique<window_wrapper>(wp)),
	gui(utki::make_shared<morda::context>(
		utki::make_shared<morda::render_opengl::renderer>(),
		utki::make_shared<morda::updater>(),
		[](std::function<void()> procedure) {
			auto& ww = get_impl(get_window_pimpl(mordavokne::inst()));
			if (PostMessage(
					ww.hwnd,
					WM_USER,
					0,
					// NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-pro-type-reinterpret-cast)
					reinterpret_cast<LPARAM>(new std::remove_reference<decltype(procedure)>::type(std::move(procedure)))
				)
				== 0)
			{
				throw std::runtime_error("PostMessage(): failed");
			}
		},
		[](morda::mouse_cursor c) {
			// TODO:
		},
		get_dots_per_inch(get_impl(this->window_pimpl).hdc),
		get_dots_per_pp(get_impl(this->window_pimpl).hdc)
	)),
	storage_dir(initialize_storage_dir(this->name)),
	curWinRect(0, 0, -1, -1)
{
	this->update_window_rect(morda::rectangle(0, 0, morda::real(wp.dims.x()), morda::real(wp.dims.y())));
}

void application::quit() noexcept
{
	auto& ww = get_impl(this->window_pimpl);
	ww.quitFlag = true;
}

namespace mordavokne {
void winmain(int argc, const char** argv)
{
	auto app = mordavokne::application_factory::get_factory()(utki::make_span(argv, argc));
	if (!app) {
		return;
	}

	ASSERT(app)

	auto& ww = get_impl(get_window_pimpl(*app));

	ShowWindow(ww.hwnd, SW_SHOW);

	while (!ww.quitFlag) {
		uint32_t timeout = app->gui.update();
		//		TRACE(<< "timeout = " << timeout << std::endl)

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

		render(*app);
		//		TRACE(<< "loop" << std::endl)
	}
}
} // namespace mordavokne

int WINAPI WinMain(
	HINSTANCE h_instance, // Instance
	HINSTANCE h_prev_instance, // Previous Instance
	LPSTR lp_cmd_line, // Command Line Parameters
	int n_cmd_show // Window Show State
)
{
	// TODO: pass argc and argv
	mordavokne::winmain(0, nullptr);

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
			GetWindowLong(ww.hwnd, GWL_EXSTYLE)
				& ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
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
			GetWindowLong(ww.hwnd, GWL_EXSTYLE)
				| (WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
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
	SwapBuffers(ww.hdc);
}

namespace {
window_wrapper::window_wrapper(const window_params& wp)
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
		"morda app",
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

	//	TRACE_AND_LOG(<<
	//"application::DeviceContextWrapper::DeviceContextWrapper(): DC created" <<
	// std::endl)

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
			wp.buffers.get(window_params::buffer_type::depth) ? BYTE(utki::byte_bits * 2)
															  : BYTE(0), // 16 bit depth buffer
			wp.buffers.get(window_params::buffer_type::stencil) ? BYTE(utki::byte_bits) : BYTE(0),
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
	scope_exit_hdc.release();
	scope_exit_hwnd.release();
	scope_exit_window_class.release();
}

window_wrapper::~window_wrapper()
{
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
