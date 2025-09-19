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
