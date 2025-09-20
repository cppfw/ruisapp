#include "window.hxx"

#include <cstdint>
#include <stdexcept>

#include <utki/windows.hpp>
#include <windowsx.h> // needed for GET_X_LPARAM macro and other similar macros

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

native_window::window_class_wrapper::window_class_wrapper() :
	name([&]() {
		return utki::cat(
			"ruisapp_window_class_name_", //
			reinterpret_cast<uintptr_t>(this)
		);
	})
{
	WNDCLASS wc;
	// TODO: get rid of memset?
	memset(&wc, 0, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // redraw on resize, own DC for window
	wc.lpfnWndProc = (WNDPROC)window_procedure; // TODO: use static_cast or no cast at all?
	wc.cbClsExtra = 0; // no extra window data
	wc.cbWndExtra = 0; // no extra window data
	wc.hInstance = GetModuleHandle(nullptr); // instance handle
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr; // no background required for OpenGL
	wc.lpszMenuName = nullptr; // we don't want a menu
	wc.lpszClassName = this->name.c_str(); // set the window class Name

	auto res = RegisterClass(&wc);

	if (res == 0) {
		// TODO: add error information to the exception message using GetLastError() and FormatMessage()
		throw std::runtime_error("Failed to register window class");
	}
}

native_window::window_class_wrapper::~window_class_wrapper()
{
	auto res = UnregisterClass(
		this->name.c_str(), //
		GetModuleHandle(nullptr)
	);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Failed to unregister window class: " << this->name;
		},
		SL
	);
}

native_window::window_wrapper::window_wrapper(const window_class_wrapper& window_class) :
	handle([&]() {
		auto hwnd = CreateWindowEx(
			WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, // extended style
			window_class.name.c_str(),
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

		if (hwnd == NULL) {
			// TODO: add error information to the exception message using GetLastError() and FormatMessage()
			throw std::runtime_error("Failed to create a window");
		}

		return hwnd;
	}())
{}

native_window::window_wrapper::~window_wrapper()
{
	auto res = DestroyWindow(this->handle);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Failed to destroy window";
		},
		SL
	);
}

native_window::device_context_wrapper::device_context_wrapper(const window_wrapper& window) :
	window(window),
	context([&]() {
		auto hdc = GetDC(this->window.handle);
		if (hdc == NULL) {
			throw std::runtime_error("Failed to create a OpenGL device context");
		}
		return hdc;
	}())
{}

native_window::device_context_wrapper::~device_context_wrapper()
{
	auto res = ReleaseDC(
		this->window.handle, //
		this->handle
	);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Failed to release device context";
		},
		SL
	);
}

#ifdef RUISAPP_RENDER_OPENGL
native_window::opengl_context_wrapper::opengl_context_wrapper(
	const device_context_wrapper& device_context,
	const ruisapp::window_parameters& window_params
) :
	context([&]() {
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
			window_params.buffers.get(ruisapp::buffer::depth) ? BYTE(utki::byte_bits * 2)
															  : BYTE(0), // 16 bit depth buffer
			window_params.buffers.get(ruisapp::buffer::stencil) ? BYTE(utki::byte_bits) : BYTE(0),
			BYTE(0), // no auxiliary buffer
			BYTE(PFD_MAIN_PLANE), // main drawing layer
			BYTE(0), // reserved
			0,
			0,
			0 // layer masks ignored
		};

		int pixel_format = ChoosePixelFormat(
			device_context.context, //
			&pfd
		);
		if (!pixel_format) {
			throw std::runtime_error("Could not find suitable pixel format");
		}

		if (!SetPixelFormat(
				device_context.context, //
				pixel_format,
				&pfd
			))
		{
			throw std::runtime_error("Could not set pixel format");
		}

		// TODO: support shared context

		// TODO: use wglCreateContextAttribsARB() which allows specifying opengl version
		auto hrc = wglCreateContext(device_context.context);
		if (hrc == NULL) {
			// TODO: add error information to the exception message using GetLastError() and FormatMessage()
			throw std::runtime_error("Failed to create OpenGL rendering context");
		}
		return hrc;
	}())
{}

native_window::opengl_context_wrapper::~opengl_context_wrapper()
{
	if (wglGetCurrentContext() == this->context) {
		// unbind context
		auto res = wglMakeCurrent(NULL, NULL);
		utki::assert(
			res,
			[&](auto& o) {
				o << "Deactivating OpenGL rendering context failed";
			},
			SL
		);
	}

	auto res = wglDeleteContext(this->context);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Releasing OpenGL rendering context failed";
		},
		SL
	);
}
#endif

native_window::native_window(
	const utki::version_duplet& gl_version,
	const ruisapp::window_parameters& window_params,
	native_window* shared_gl_context_native_window
) :
	window(this->window_class),
	device_context(this->window),
#ifdef RUISAPP_RENDER_OPENGL
	opengl_context(
		this->device_context, //
		window_params
	)
#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_display(EGLNativeDisplayType(this->device_context.context)),
	egl_config(
		this->egl_display, //
		gl_version,
		window_params
	),
	egl_surface(this->egl_display, this->egl_config, EGLNativeWindowType(this->window.handle)),
	egl_context(
		this->egl_display,
		gl_version,
		this->egl_config,
		shared_gl_context_native_window ? shared_gl_context_native_window->egl_context.context : EGL_NO_CONTEXT
	)
#endif
{
#ifdef RUISAPP_RENDER_OPENGL
	if (wglGetCurrentContext() == NULL) {
		auto res = wglMakeCurrent(
			this->device_context.context, //
			this->opengl_context.context
		);

		if (!res) {
			throw std::runtime_error("Failed to activate OpenGL rendering context");
		}

		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("GLEW initialization failed");
		}
	}
#endif
}

void native_window::swap_frame_buffers()
{
#ifdef RUISAPP_RENDER_OPENGL
	SwapBuffers(this->device_context.context);
#elif defined(RUISAPP_RENDER_OPENGLES)
	eglSwapBuffers(
		this->egl_display.display, //
		this->egl_surface.surface
	);
#else
#	error "Unknown graphics API"
#endif
}
