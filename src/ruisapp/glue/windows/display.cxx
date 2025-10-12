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

#include <stdexcept>

#include <utki/debug.hpp>
#include <utki/enum_iterable.hpp>
#include <utki/windows.hpp>
#include <windowsx.h> // needed for GET_X_LPARAM macro and other similar macros

#include "application.hxx"
#include "key_code_map.hxx"

namespace {
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
LRESULT CALLBACK window_procedure(
	HWND hwnd, //
	UINT msg,
	WPARAM w_param,
	LPARAM l_param
)
{
	auto& glue = get_glue();

	auto* window = glue.get_window(hwnd);
	if (!window) {
		// CreateWindowEx() calls window procedure while creating the window,
		// at this moment, the app_window object is not yet created, so perform
		// default window procedure in this case.
		return DefWindowProc(
			hwnd, //
			msg,
			w_param,
			l_param
		);
	}

	auto& win = *window;

	auto& natwin = win.ruis_native_window.get();

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
					[[fallthrough]];
				case SC_MONITORPOWER: // montor trying to enter powersave?
					return 0; // prevent from happening
				default:
					break;
			}
			break;

		case WM_CLOSE:
			if (natwin.close_handler) {
				natwin.close_handler();
			}
			return 0;

		case WM_MOUSEMOVE:
			{
				if (!win.is_hovered) {
					TRACKMOUSEEVENT tme = {sizeof(tme)};
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hwnd;
					TrackMouseEvent(&tme);

					win.is_hovered = true;

					// restore mouse cursor invisibility
					if (!natwin.is_mouse_cursor_visible()) {
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

					win.gui.send_mouse_hover(
						true, //
						0 // pointer id
					);
				}
				win.gui.send_mouse_move(
					ruis::vector2(
						float(GET_X_LPARAM(l_param)), //
						float(GET_Y_LPARAM(l_param))
					),
					0 // pointer id
				);
				return 0;
			}
		case WM_MOUSELEAVE:
			{
				// Windows hides the mouse cursor even in non-client areas of the window,
				// like caption bar and borders, so show cursor if it is hidden
				if (!natwin.is_mouse_cursor_visible()) {
					ShowCursor(TRUE);
				}

				win.is_hovered = false;
				win.gui.send_mouse_hover(
					false,
					0 // pointer id
				);

				// Report mouse button up events for all pressed mouse buttons
				for (auto btn : utki::enum_iterable_v<decltype(win.mouse_button_state)::enum_type>) {
					if (win.mouse_button_state.get(btn)) {
						win.mouse_button_state.clear(btn);
						constexpr auto outside_of_window_coordinate = 100000000;
						win.gui.send_mouse_button(
							false,
							ruis::vector2(
								outside_of_window_coordinate, //
								outside_of_window_coordinate
							),
							btn,
							0 // pointer id
						);
					}
				}
				return 0;
			}
		case WM_LBUTTONDOWN:
			{
				win.mouse_button_state.set(ruis::mouse_button::left);
				win.gui.send_mouse_button(
					true,
					ruis::vector2(
						float(GET_X_LPARAM(l_param)), //
						float(GET_Y_LPARAM(l_param))
					),
					ruis::mouse_button::left,
					0 // pointer id
				);
				return 0;
			}
		case WM_LBUTTONUP:
			{
				win.mouse_button_state.clear(ruis::mouse_button::left);
				win.gui.send_mouse_button(
					false,
					ruis::vector2(
						float(GET_X_LPARAM(l_param)), //
						float(GET_Y_LPARAM(l_param))
					),
					ruis::mouse_button::left,
					0 // pointer id
				);
				return 0;
			}
		case WM_MBUTTONDOWN:
			{
				win.mouse_button_state.set(ruis::mouse_button::middle);
				win.gui.send_mouse_button(
					true,
					ruis::vector2(
						float(GET_X_LPARAM(l_param)), //
						float(GET_Y_LPARAM(l_param))
					),
					ruis::mouse_button::middle,
					0 // pointer id
				);
				return 0;
			}
		case WM_MBUTTONUP:
			{
				win.mouse_button_state.clear(ruis::mouse_button::middle);
				win.gui.send_mouse_button(
					false,
					ruis::vector2(
						float(GET_X_LPARAM(l_param)), //
						float(GET_Y_LPARAM(l_param))
					),
					ruis::mouse_button::middle,
					0 // pointer id
				);
				return 0;
			}
		case WM_RBUTTONDOWN:
			{
				win.mouse_button_state.set(ruis::mouse_button::right);
				win.gui.send_mouse_button(
					true,
					ruis::vector2(
						float(GET_X_LPARAM(l_param)), //
						float(GET_Y_LPARAM(l_param))
					),
					ruis::mouse_button::right,
					0 // pointer id
				);
				return 0;
			}
		case WM_RBUTTONUP:
			{
				win.mouse_button_state.clear(ruis::mouse_button::right);
				win.gui.send_mouse_button(
					false,
					ruis::vector2(
						float(GET_X_LPARAM(l_param)), //
						float(GET_Y_LPARAM(l_param))
					),
					ruis::mouse_button::right,
					0 // pointer id
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
					win.gui.send_mouse_button(
						true, //
						ruis::vector2(
							float(pos.x), //
							float(pos.y)
						),
						button,
						0 // pointer id
					);
					win.gui.send_mouse_button(
						false, //
						ruis::vector2(
							float(pos.x), //
							float(pos.y)
						),
						button,
						0 // pointer id
					);
				}
			}
			return 0;

		case WM_KEYDOWN:
			{
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				ruis::key key = key_code_map[uint8_t(w_param)];

				constexpr auto previous_key_state_mask = 0x40000000;

				if ((l_param & previous_key_state_mask) == 0) { // ignore auto-repeated keypress event
					win.gui.send_key(
						true, //
						key
					);
				}
				win.gui.send_character_input(
					windows_input_string_provider(), //
					key
				);
				return 0;
			}
		case WM_KEYUP:
			win.gui.send_key(
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
					win.gui.send_character_input(windows_input_string_provider(char32_t(w_param)), ruis::key::unknown);
					break;
			}
			return 0;
		case WM_PAINT:
			// we will redraw anyway on every cycle

			// Tell Windows that we have redrawn contents
			// and WM_PAINT message should go away from message queue.
			ValidateRect(
				hwnd, //
				NULL
			);
			return 0;

		case WM_SIZE:
			win.gui.set_viewport( //
				ruis::rect(
					0, //
					0,
					ruis::real(LOWORD(l_param)), // width
					ruis::real(HIWORD(l_param)) // height
				)
			);
			return 0;

		default:
			break;
	}

	return DefWindowProc(
		hwnd, //
		msg,
		w_param,
		l_param
	);
}
} // namespace

display_wrapper::window_class_wrapper::window_class_wrapper(
	const char* window_class_name, //
	WNDPROC window_procedure
) :
	window_class_name(window_class_name)
{
	WNDCLASSA wc;

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // redraw on resize, own DC for window
	wc.lpfnWndProc = &window_procedure;
	wc.cbClsExtra = 0; // no extra window data
	wc.cbWndExtra = 0; // no extra window data
	wc.hInstance = GetModuleHandle(nullptr); // instance handle
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr; // no background needed for OpenGL
	wc.lpszMenuName = nullptr; // we don't want a menu
	wc.lpszClassName = this->window_class_name; // set the window class name

	auto res = RegisterClassA(&wc);

	if (res == 0) {
		// TODO: add error information to the exception message using GetLastError() and FormatMessage()
		throw std::runtime_error("Failed to register window class");
	}
}

display_wrapper::window_class_wrapper::~window_class_wrapper()
{
	auto res = UnregisterClassA(
		this->window_class_name, //
		GetModuleHandle(nullptr)
	);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Failed to unregister window class: " << this->window_class_name;
		},
		SL
	);
}

namespace {
const char* dummy_window_class_name = "ruisapp_dummy_window_class_name";
} // namespace

#ifdef RUISAPP_RENDER_OPENGL
display_wrapper::wgl_procedures_wrapper::wgl_procedures_wrapper()
{
	HWND dummy_window = CreateWindowExA(
		0,
		dummy_window_class_name,
		"",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		GetModuleHandle(NULL),
		0
	);

	if (!dummy_window) {
		throw std::runtime_error("CreateWindowExA() failed");
	}

	utki::scope_exit window_scope_exit([&]() {
		DestroyWindow(dummy_window);
	});

	HDC dummy_dc = GetDC(dummy_window);
	if (dummy_dc == NULL) {
		throw std::runtime_error("GetDC() failed");
	}
	utki::scope_exit device_context_scope_exit([&]() {
		ReleaseDC(
			dummy_window, //
			dummy_dc
		);
	});

	PIXELFORMATDESCRIPTOR pfd = {
		.nSize = sizeof(pfd),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cAlphaBits = 8,
		.cDepthBits = 24,
		.cStencilBits = 8,
		.iLayerType = PFD_MAIN_PLANE
	};

	int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
	if (!pixel_format) {
		throw std::runtime_error("ChoosePixelFormat() failed");
	}
	if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
		throw std::runtime_error("SetPixelFormat() failed");
	}

	HGLRC dummy_context = wglCreateContext(dummy_dc);
	if (dummy_context == NULL) {
		throw std::runtime_error("wglCreateContext() failed");
	}

	utki::scope_exit rendering_context_scope_exit([&]() {
		wglMakeCurrent(
			dummy_dc, //
			NULL
		);
		wglDeleteContext(dummy_context);
	});

	if (!wglMakeCurrent(dummy_dc, dummy_context)) {
		throw std::runtime_error("wglMakeCurrent() failed");
	}

	this->wgl_choose_pixel_format_arb = PFNWGLCHOOSEPIXELFORMATARBPROC(wglGetProcAddress("wglChoosePixelFormatARB"));
	if (!this->wgl_choose_pixel_format_arb) {
		throw std::runtime_error("could not get wglChoosePixelFormatARB()");
	}
	this->wgl_create_context_attribs_arb =
		PFNWGLCREATECONTEXTATTRIBSARBPROC(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (!this->wgl_create_context_attribs_arb) {
		throw std::runtime_error("could not get wglCreateContextAttribsARB()");
	}
}
#endif

display_wrapper::display_wrapper() :
	dummy_window_class(
		dummy_window_class_name, //
		DefWindowProc
	),
	regular_window_class(
		"ruisapp_window_class_name", //
		window_procedure
	)
{}
