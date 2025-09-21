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

#include <papki/fs_file.hpp>
#include <ruis/context.hpp>
#include <ruis/util/util.hpp>
#include <utki/destructable.hpp>
#include <utki/windows.hpp>

#ifdef RUISAPP_RENDER_OPENGL
#	include <ruis/render/opengl/context.hpp>
#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <ruis/render/opengles/context.hpp>
#else
#	error "Unknown graphics API"
#endif

#include "../../application.hpp"

#include "application.hxx"

// include implementations
#include "display.cxx"
#include "application.cxx"
#include "window.cxx"

using namespace ruisapp;

//namespace {
//constexpr const char* window_class_name = "RuisappWindowClassName";
//} // namespace

//namespace {
//struct window_wrapper : public utki::destructable {
//	HWND hwnd;
//	HDC hdc;
//
//#ifdef RUISAPP_RENDER_OPENGL
//	HGLRC hrc;
//#elif defined(RUISAPP_RENDER_OPENGLES)
//	EGLDisplay egl_display;
//	EGLSurface egl_surface;
//	EGLContext egl_context;
//#else
//#	error "Unknown graphics API"
//#endif
//
//	bool isHovered = false; // for tracking when mouse enters or leaves window.
//
//	utki::flags<ruis::mouse_button> mouseButtonState;
//
//	bool mouseCursorIsCurrentlyVisible = true;
//
//	window_wrapper(const window_parameters& wp);
//
//	window_wrapper(const window_wrapper&) = delete;
//	window_wrapper& operator=(const window_wrapper&) = delete;
//
//	window_wrapper(window_wrapper&&) = delete;
//	window_wrapper& operator=(window_wrapper&&) = delete;
//
//	~window_wrapper() override;
//};

// window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
// {
// 	ASSERT(dynamic_cast<window_wrapper*>(pimpl.get()))
// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
// 	return static_cast<window_wrapper&>(*pimpl);
// }
//} // namespace

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

// TODO:
// application::application(
// 	std::string name, //
// 	const window_parameters& wp
// ) :
// 	name(std::move(name)),
// 	window_pimpl(std::make_unique<window_wrapper>(wp)),
// 	gui(utki::make_shared<ruis::context>(
// 		utki::make_shared<ruis::style_provider>( //
// 			utki::make_shared<ruis::resource_loader>( //
// 				utki::make_shared<ruis::render::renderer>( //
// #ifdef RUISAPP_RENDER_OPENGL
// 					utki::make_shared<ruis::render::opengl::context>()
// #elif defined(RUISAPP_RENDER_OPENGLES)
// 					utki::make_shared<ruis::render::opengles::context>()
// #else
// #	error "Unknown graphics API"
// #endif
// 				)
// 			)
// 		),
// 		utki::make_shared<ruis::updater>(),
// 		ruis::context::parameters{
// 			.post_to_ui_thread_function =
// 				[](std::function<void()> procedure) {
// 					auto& ww = get_impl(get_window_pimpl(ruisapp::inst()));
// 					if (PostMessage(
// 							ww.hwnd,
// 							WM_USER,
// 							0,
// 							// NOLINTNEXTLINE(cppcoreguidelines-owning-memory,
// 							// cppcoreguidelines-pro-type-reinterpret-cast)
// 							reinterpret_cast<LPARAM>(
// 								new std::remove_reference_t<decltype(procedure)>(std::move(procedure))
// 							)
// 						) == 0)
// 					{
// 						throw std::runtime_error("PostMessage(): failed");
// 					}
// 				},
// 			.set_mouse_cursor_function =
// 				[](ruis::mouse_cursor c) {
// 					// TODO:
// 				},
// 			.units = ruis::units(
// 				get_dots_per_inch(get_impl(this->window_pimpl).hdc), //
// 				get_dots_per_pp(get_impl(this->window_pimpl).hdc)
// 			)
// 		}
// 	)),
// 	directory(get_application_directories(this->name)),
// 	cur_window_rect(0, 0, -1, -1)
// {
// 	this->update_window_rect(ruis::rect(0, 0, ruis::real(wp.dims.x()), ruis::real(wp.dims.y())));
// }

// TODO:
// void application::quit() noexcept
// {
// 	auto& ww = get_impl(this->window_pimpl);
// 	ww.quitFlag = true;
// }

namespace {
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

	auto& glue = get_glue(*app);

	while (!glue.quit_flag.load()) {
		glue.windows_to_destroy.clear();

		// main loop cycle sequence as required by ruis:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle
		uint32_t timeout = glue.updater.get().update();

		glue.render();

		DWORD status = MsgWaitForMultipleObjectsEx(
			0,// number of handles to wait for
			nullptr,// we do not wait for any handles
			timeout,
			QS_ALLINPUT, // we wait for ALL inpuit events
			MWMO_INPUTAVAILABLE // we wait for inpuit events
		);

		if (status == WAIT_OBJECT_0) {
			// not a timeout, some events happened

			MSG msg;
			while (PeekMessage(
				&msg,//
				NULL, // retrieve messages for any window, as well as thread messages
				0, // no message filtering, retrieve all messages
				0, // no message filtering, retrieve all messages
				PM_REMOVE // remove messages from queue after processing by PeekMessage()
			)) {
				if (msg.message == WM_QUIT) {
					glue.quit_flag.store(true);
					break;
				}else if (msg.message == WM_USER)
				{
					std::unique_ptr<std::function<void()>> m(
						// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
						reinterpret_cast<std::function<void()>*>(msg.lParam)
					);
					(*m)();
					continue;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
}
} // namespace

int WINAPI WinMain(
	HINSTANCE h_instance, // Instance
	HINSTANCE h_prev_instance, // Previous Instance
	LPSTR lp_cmd_line, // Command Line Parameters
	int n_cmd_show // Window Show State
)
{
	winmain(
		__argc, //
		const_cast<const char**>(__argv)
	);

	return 0;
}

int main(int argc, const char** argv)
{
	winmain(argc, argv);

	return 0;
}

// TODO:
// void application::set_fullscreen(bool enable)
// {
// 	if (enable == this->is_fullscreen()) {
// 		return;
// 	}

// 	auto& ww = get_impl(this->window_pimpl);

// 	if (enable) {
// 		// save original window size
// 		RECT rect;
// 		if (GetWindowRect(ww.hwnd, &rect) == 0) {
// 			throw std::runtime_error("Failed to get window rect");
// 		}
// 		this->before_fullscreen_window_rect.p.x() = rect.left;
// 		this->before_fullscreen_window_rect.p.y() = rect.top;
// 		this->before_fullscreen_window_rect.d.x() = rect.right - rect.left;
// 		this->before_fullscreen_window_rect.d.y() = rect.bottom - rect.top;

// 		// Set new window style
// 		SetWindowLong(ww.hwnd, GWL_STYLE, GetWindowLong(ww.hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME));
// 		SetWindowLong(
// 			ww.hwnd,
// 			GWL_EXSTYLE,
// 			GetWindowLong(ww.hwnd, GWL_EXSTYLE) &
// 				~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
// 		);

// 		// set new window size and position
// 		MONITORINFO mi;
// 		mi.cbSize = sizeof(mi);
// 		GetMonitorInfo(MonitorFromWindow(ww.hwnd, MONITOR_DEFAULTTONEAREST), &mi);
// 		SetWindowPos(
// 			ww.hwnd,
// 			nullptr,
// 			mi.rcMonitor.left,
// 			mi.rcMonitor.top,
// 			mi.rcMonitor.right - mi.rcMonitor.left,
// 			mi.rcMonitor.bottom - mi.rcMonitor.top,
// 			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
// 		);
// 	} else {
// 		// Reset original window style
// 		SetWindowLong(ww.hwnd, GWL_STYLE, GetWindowLong(ww.hwnd, GWL_STYLE) | (WS_CAPTION | WS_THICKFRAME));
// 		SetWindowLong(
// 			ww.hwnd,
// 			GWL_EXSTYLE,
// 			GetWindowLong(ww.hwnd, GWL_EXSTYLE) |
// 				(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
// 		);

// 		SetWindowPos(
// 			ww.hwnd,
// 			nullptr,
// 			this->before_fullscreen_window_rect.p.x(),
// 			this->before_fullscreen_window_rect.p.y(),
// 			this->before_fullscreen_window_rect.d.x(),
// 			this->before_fullscreen_window_rect.d.y(),
// 			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
// 		);
// 	}

// 	this->is_fullscreen_v = enable;
// }

// TODO:
// void application::set_mouse_cursor_visible(bool visible)
// {
// 	auto& ww = get_impl(this->window_pimpl);

// 	if (visible) {
// 		if (!ww.mouseCursorIsCurrentlyVisible) {
// 			ShowCursor(TRUE);
// 			ww.mouseCursorIsCurrentlyVisible = true;
// 		}
// 	} else {
// 		if (ww.mouseCursorIsCurrentlyVisible) {
// 			ShowCursor(FALSE);
// 			ww.mouseCursorIsCurrentlyVisible = false;
// 		}
// 	}
// }

// namespace {
// window_wrapper::window_wrapper(const window_parameters& wp)
// {
// 	// NOTE: window will be shown later, right before entering main loop and after
// 	// all initial App data is initialized

// #ifdef RUISAPP_RENDER_OPENGL
// #elif defined(RUISAPP_RENDER_OPENGLES)

// 	auto graphics_api_version = [&ver = wp.graphics_api_version]() {
// 		if (ver.to_uint32_t() == 0) {
// 			// default OpenGL ES version is 2.0
// 			return utki::version_duplet{
// 				.major = 2, //
// 				.minor = 0
// 			};
// 		}
// 		return ver;
// 	}();

// 	this->egl_display = eglGetDisplay(this->hdc);
// 	if (this->egl_display == EGL_NO_DISPLAY) {
// 		throw std::runtime_error("Failed to get EGL display");
// 	}

// 	if (!eglInitialize(this->egl_display, nullptr, nullptr)) {
// 		throw std::runtime_error("Failed to initialize EGL");
// 	}

// 	utki::scope_exit scope_exit_display([this]() {
// 		if (!eglTerminate(this->egl_display)) {
// 			ASSERT(false, [&](auto& o) {
// 				o << "Terminating EGL failed";
// 			})
// 		}
// 	});

// 	EGLConfig config = nullptr;
// 	{
// 		const std::array<EGLint, 15> config_attribs = {
// 			EGL_SURFACE_TYPE,
// 			EGL_WINDOW_BIT,
// 			EGL_RENDERABLE_TYPE,
// 			// We cannot set bits for all OpenGL ES versions because on platforms which do not
// 			// support later versions the matching config will not be found by eglChooseConfig().
// 			// So, set bits according to requested OpenGL ES version.
// 			[&ver = wp.graphics_api_version]() {
// 				EGLint ret = EGL_OPENGL_ES2_BIT; // OpenGL ES 2 is the minimum
// 				if (ver.major >= 3) {
// 					ret |= EGL_OPENGL_ES3_BIT;
// 				}
// 				return ret;
// 			}(),
// 			EGL_RED_SIZE,
// 			8,
// 			EGL_GREEN_SIZE,
// 			8,
// 			EGL_BLUE_SIZE,
// 			8,
// 			EGL_DEPTH_SIZE,
// 			wp.buffers.get(ruisapp::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
// 			EGL_STENCIL_SIZE,
// 			wp.buffers.get(ruisapp::buffer::stencil) ? utki::byte_bits : 0,
// 			EGL_NONE
// 		};

// 		EGLint num_configs = 0;
// 		if (!eglChooseConfig(this->egl_display, config_attribs.data(), &config, 1, &num_configs)) {
// 			throw std::runtime_error("Failed to choose EGL config");
// 		}
// 	}

// 	this->egl_surface = eglCreateWindowSurface(
// 		this->egl_display, //
// 		config,
// 		EGLNativeWindowType(this->hwnd),
// 		nullptr
// 	);
// 	if (this->egl_surface == EGL_NO_SURFACE) {
// 		throw std::runtime_error("Failed to create EGL surface");
// 	}

// 	utki::scope_exit scope_exit_surface([this]() {
// 		if (!eglDestroySurface(this->egl_display, this->egl_surface)) {
// 			ASSERT(false, [&](auto& o) {
// 				o << "Destroying EGL surface failed";
// 			})
// 		}
// 	});

// 	{
// 		constexpr auto attrs_array_size = 5;
// 		std::array<EGLint, attrs_array_size> context_attrs = {
// 			EGL_CONTEXT_MAJOR_VERSION,
// 			graphics_api_version.major,
// 			EGL_CONTEXT_MINOR_VERSION,
// 			graphics_api_version.minor,
// 			EGL_NONE
// 		};

// 		this->egl_context = eglCreateContext(this->egl_display, config, EGL_NO_CONTEXT, context_attrs.data());
// 		if (this->egl_context == EGL_NO_CONTEXT) {
// 			throw std::runtime_error("Failed to create EGL context");
// 		}
// 	}

// 	utki::scope_exit scope_exit_context([this]() {
// 		if (!eglDestroyContext(this->egl_display, this->egl_context)) {
// 			ASSERT(false, [&](auto& o) {
// 				o << "Destroying EGL context failed";
// 			})
// 		}
// 	});

// 	if (!eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context)) {
// 		throw std::runtime_error("Failed to make EGL context current");
// 	}

// 	scope_exit_context.release();
// 	scope_exit_surface.release();
// 	scope_exit_display.release();
// #else
// #	error "Unknown graphics API"
// #endif
// }

// window_wrapper::~window_wrapper()
// {
// #ifdef RUISAPP_RENDER_OPENGL
// #elif defined(RUISAPP_RENDER_OPENGLES)

// 	if (!eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
// 		ASSERT(false, [&](auto& o) {
// 			o << "Deactivating EGL context failed";
// 		})
// 	}

// 	if (!eglDestroyContext(this->egl_display, this->egl_context)) {
// 		ASSERT(false, [&](auto& o) {
// 			o << "Destroying EGL context failed";
// 		})
// 	}

// 	if (!eglDestroySurface(this->egl_display, this->egl_surface)) {
// 		ASSERT(false, [&](auto& o) {
// 			o << "Destroying EGL surface failed";
// 		})
// 	}

// 	if (!eglTerminate(this->egl_display)) {
// 		ASSERT(false, [&](auto& o) {
// 			o << "Terminating EGL failed";
// 		})
// 	}

// #else
// #	error "Unknown graphics API"
// #endif
// }
// } // namespace
