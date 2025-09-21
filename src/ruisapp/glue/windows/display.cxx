#include "display.hxx"

#include <stdexcept>

#include <utki/windows.hpp>
#include <utki/debug.hpp>

#include <windowsx.h> // needed for GET_X_LPARAM macro and other similar macros

#include "application.hxx"

namespace {
LRESULT CALLBACK window_procedure(HWND hwnd,//
	UINT msg, WPARAM w_param, LPARAM l_param)
{
	auto& glue = get_glue();

	auto* window = glue.get_window(hwnd);
	if (!window) {
		// CreateWindowEx() calls window procedure while creating the window,
		// at this moment, the app_window object is not yet created, so perform
		// default window procedure in this case.
		return DefWindowProc(hwnd,//
		msg, w_param, l_param);
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
				//auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				// TODO:
				//if (!ww.isHovered) {
				//	TRACKMOUSEEVENT tme = {sizeof(tme)};
				//	tme.dwFlags = TME_LEAVE;
				//	tme.hwndTrack = hwnd;
				//	TrackMouseEvent(&tme);

				//	ww.isHovered = true;

				//	// restore mouse cursor invisibility
				//	if (!ww.mouseCursorIsCurrentlyVisible) {
				//		CURSORINFO ci;
				//		ci.cbSize = sizeof(CURSORINFO);
				//		if (GetCursorInfo(&ci) != 0) {
				//			if (ci.flags & CURSOR_SHOWING) {
				//				ShowCursor(FALSE);
				//			}
				//		} else {
				//			utki::log_debug([&](auto& o) {
				//				o << "GetCursorInfo(): failed!!!" << std::endl;
				//			});
				//		}
				//	}

				//	handle_mouse_hover(ruisapp::inst(), true, 0);
				//}
				win.gui.send_mouse_move(
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					0
				);
				return 0;
			}
		case WM_MOUSELEAVE:
			{
				// TODO:
				//auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));

				//// Windows hides the mouse cursor even in non-client areas of the window,
				//// like caption bar and borders, so show cursor if it is hidden
				//if (!ww.mouseCursorIsCurrentlyVisible) {
				//	ShowCursor(TRUE);
				//}

				//ww.isHovered = false;
				//handle_mouse_hover(ruisapp::inst(), false, 0);

				//// Report mouse button up events for all pressed mouse buttons
				//for (size_t i = 0; i != ww.mouseButtonState.size(); ++i) {
				//	auto btn = ruis::mouse_button(i);
				//	if (ww.mouseButtonState.get(btn)) {
				//		ww.mouseButtonState.clear(btn);
				//		constexpr auto outside_of_window_coordinate = 100000000;
				//		handle_mouse_button(
				//			ruisapp::inst(),
				//			false,
				//			ruis::vector2(outside_of_window_coordinate, outside_of_window_coordinate),
				//			btn,
				//			0
				//		);
				//	}
				//}
				return 0;
			}
		case WM_LBUTTONDOWN:
			{
				// TDOO:
				//auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				//ww.mouseButtonState.set(ruis::mouse_button::left);
				//handle_mouse_button(
				//	ruisapp::inst(),
				//	true,
				//	ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
				//	ruis::mouse_button::left,
				//	0
				//);
				return 0;
			}
		case WM_LBUTTONUP:
			{
				// TODO:
				//auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				//ww.mouseButtonState.clear(ruis::mouse_button::left);
				//handle_mouse_button(
				//	ruisapp::inst(),
				//	false,
				//	ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
				//	ruis::mouse_button::left,
				//	0
				//);
				return 0;
			}
		case WM_MBUTTONDOWN:
			{
				// TODO:
				/*auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.set(ruis::mouse_button::middle);
				handle_mouse_button(
					ruisapp::inst(),
					true,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::middle,
					0
				);*/
				return 0;
			}
		case WM_MBUTTONUP:
			{
				// TODO:
				/*auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.clear(ruis::mouse_button::middle);
				handle_mouse_button(
					ruisapp::inst(),
					false,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::middle,
					0
				);*/
				return 0;
			}
		case WM_RBUTTONDOWN:
			{
				// TODO:
				/*auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.set(ruis::mouse_button::right);
				handle_mouse_button(
					ruisapp::inst(),
					true,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::right,
					0
				);*/
				return 0;
			}
		case WM_RBUTTONUP:
			{
				// TODO:
				/*auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
				ww.mouseButtonState.clear(ruis::mouse_button::right);
				handle_mouse_button(
					ruisapp::inst(),
					false,
					ruis::vector2(float(GET_X_LPARAM(l_param)), float(GET_Y_LPARAM(l_param))),
					ruis::mouse_button::right,
					0
				);*/
				return 0;
			}
		case WM_MOUSEWHEEL:
			[[fallthrough]];
		case WM_MOUSEHWHEEL:
			// TODO:
			//{
			//	unsigned short int times = HIWORD(w_param);
			//	times /= WHEEL_DELTA;
			//	ruis::mouse_button button = [&times, &msg]() {
			//		if (times >= 0) {
			//			return msg == WM_MOUSEWHEEL ? ruis::mouse_button::wheel_up : ruis::mouse_button::wheel_right;
			//		} else {
			//			times = -times;
			//			return msg == WM_MOUSEWHEEL ? ruis::mouse_button::wheel_down : ruis::mouse_button::wheel_left;
			//		}
			//	}();

			//	POINT pos;
			//	pos.x = GET_X_LPARAM(l_param);
			//	pos.y = GET_Y_LPARAM(l_param);

			//	// For some reason in WM_MOUSEWHEEL message mouse cursor position is sent in
			//	// screen coordinates, need to traslate those to window coordinates.
			//	if (ScreenToClient(hwnd, &pos) == 0) {
			//		break;
			//	}

			//	for (unsigned i = 0; i != times; ++i) {
			//		handle_mouse_button(ruisapp::inst(), true, ruis::vector2(float(pos.x), float(pos.y)), button, 0);
			//		handle_mouse_button(ruisapp::inst(), false, ruis::vector2(float(pos.x), float(pos.y)), button, 0);
			//	}
			//}
			return 0;

		case WM_KEYDOWN:
			{
				// TODO:
				//// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
				//ruis::key key = key_code_map[uint8_t(w_param)];

				//constexpr auto previous_key_state_mask = 0x40000000;

				//if ((l_param & previous_key_state_mask) == 0) { // ignore auto-repeated keypress event
				//	handle_key_event(ruisapp::inst(), true, key);
				//}
				//handle_character_input(ruisapp::inst(), windows_input_string_provider(), key);
				return 0;
			}
		case WM_KEYUP:
			// TODO:
			//handle_key_event(
			//	ruisapp::inst(),
			//	false,
			//	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
			//	key_code_map[std::uint8_t(w_param)]
			//);
			return 0;

		case WM_CHAR:
			// TODO:
			//switch (char32_t(w_param)) {
			//	case U'\U00000008': // Backspace character
			//	case U'\U0000001b': // Escape character
			//	case U'\U0000000d': // Carriage return
			//		break;
			//	default:
			//		handle_character_input(
			//			ruisapp::inst(),
			//			windows_input_string_provider(char32_t(w_param)),
			//			ruis::key::unknown
			//		);
			//		break;
			//}
			return 0;
		case WM_PAINT:
			// we will redraw anyway on every cycle
			// app.Render();

			// Tell Windows that we have redrawn contents
			// and WM_PAINT message should go away from message queue.
			ValidateRect(hwnd, //
				NULL);
			return 0;

		case WM_SIZE:
			// LoWord=Width, HiWord=Height
			 
			// TODO:
			//update_window_rect(
			//	ruisapp::inst(), //
			//	ruis::rect(0, 0, ruis::real(LOWORD(l_param)), ruis::real(HIWORD(l_param)))
			//);
			return 0;

		default:
			break;
	}

	return DefWindowProc(hwnd,//
		msg, w_param, l_param);
}
} // namespace

display_wrapper::window_class_wrapper::window_class_wrapper(
	const char* window_class_name,
		WNDPROC window_procedure
) :
	window_class_name(window_class_name)
{
	WNDCLASS wc;
	// TODO: get rid of memset?
	memset(&wc, 0, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // redraw on resize, own DC for window
	wc.lpfnWndProc = window_procedure;
	wc.cbClsExtra = 0; // no extra window data
	wc.cbWndExtra = 0; // no extra window data
	wc.hInstance = GetModuleHandle(nullptr); // instance handle
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr; // no background required for OpenGL
	wc.lpszMenuName = nullptr; // we don't want a menu
	wc.lpszClassName = this->window_class_name; // set the window class name

	auto res = RegisterClass(&wc);

	if (res == 0) {
		// TODO: add error information to the exception message using GetLastError() and FormatMessage()
		throw std::runtime_error("Failed to register window class");
	}
}

display_wrapper::window_class_wrapper::~window_class_wrapper()
{
	auto res = UnregisterClass(
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
}

#ifdef RUISAPP_RENDER_OPENGL
display_wrapper::wgl_procedures_wrapper::wgl_procedures_wrapper() {
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
        0);

    if (!dummy_window) {
        throw std::runtime_error("CreateWindowExA() failed");
    }

	utki::scope_exit window_scope_exit([&](){
		DestroyWindow(dummy_window);
	});

    HDC dummy_dc = GetDC(dummy_window);
	if (dummy_dc == NULL) {
		throw std::runtime_error("GetDC() failed");
	}
	utki::scope_exit device_context_scope_exit([&]() {
		ReleaseDC(dummy_window, //
			dummy_dc);
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

	utki::scope_exit rendering_context_scope_exit([&](){
		wglMakeCurrent(dummy_dc,//
			NULL);
    wglDeleteContext(dummy_context);
		});

    if (!wglMakeCurrent(dummy_dc, dummy_context)) {
        throw std::runtime_error("wglMakeCurrent() failed");
    }

	this->wgl_choose_pixel_format_arb = PFNWGLCHOOSEPIXELFORMATARBPROC(wglGetProcAddress("wglChoosePixelFormatARB"));   
	if (!this->wgl_choose_pixel_format_arb) {
		throw std::runtime_error("could not get wglChoosePixelFormatARB()");
	}
	this->wgl_create_context_attribs_arb= PFNWGLCREATECONTEXTATTRIBSARBPROC(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (!this->wgl_create_context_attribs_arb) {
		throw std::runtime_error("could not get wglCreateContextAttribsARB()");
	}
}
#endif

display_wrapper::display_wrapper() :
	dummy_window_class(dummy_window_class_name,//
		DefWindowProc),
	regular_window_class("ruisapp_window_class_name", //
		window_procedure)
{}
