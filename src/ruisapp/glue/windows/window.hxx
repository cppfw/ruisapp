#pragma once

#include <string>

#include <ruis/render/native_window.hpp>
#include <utki/version.hpp>
#include <utki/windows.hpp>

#include "../../window.hpp"

#ifdef RUISAPP_RENDER_OPENGLES
#	include "../egl_utils.hxx"
#endif

#include "display.hxx"

namespace {
class native_window : public ruis::render::native_window
{
	utki::shared_ref<display_wrapper> display;

	struct window_wrapper {
		const HWND handle;

		window_wrapper(
			const display_wrapper::window_class_wrapper& window_class,
			const ruisapp::window_parameters& window_params,
			bool visible
		);

		window_wrapper(const window_wrapper&) = delete;
		window_wrapper& operator=(const window_wrapper&) = delete;

		window_wrapper(window_wrapper&&) = delete;
		window_wrapper& operator=(window_wrapper&&) = delete;

		~window_wrapper();
	} window;

	struct device_context_wrapper {
		const window_wrapper& window;

		const HDC context;

		device_context_wrapper(const window_wrapper& window);

		device_context_wrapper(const device_context_wrapper&) = delete;
		device_context_wrapper& operator=(const device_context_wrapper&) = delete;

		device_context_wrapper(device_context_wrapper&&) = delete;
		device_context_wrapper& operator=(device_context_wrapper&&) = delete;

		~device_context_wrapper();
	} device_context;

#ifdef RUISAPP_RENDER_OPENGL
	struct opengl_context_wrapper {
		const HGLRC context;

		opengl_context_wrapper(
			const display_wrapper& display, //
			const device_context_wrapper& device_context,
			const ruisapp::window_parameters& window_params,
			const utki::version_duplet& gl_version,
			HGLRC shared_context
		);

		opengl_context_wrapper(const opengl_context_wrapper&) = delete;
		opengl_context_wrapper& operator=(const opengl_context_wrapper&) = delete;

		opengl_context_wrapper(opengl_context_wrapper&&) = delete;
		opengl_context_wrapper& operator=(opengl_context_wrapper&&) = delete;

		~opengl_context_wrapper();
	} opengl_context;
#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_config_wrapper egl_config;
	egl_surface_wrapper egl_surface;
	egl_context_wrapper egl_context;
#endif

	bool mouse_cursor_visible = true;

	r4::rectangle<int> before_fullscreen_window_rect{0, 0, 0, 0};

public:
	using window_id_type = HWND;

	window_id_type get_id() const noexcept
	{
		return this->window.handle;
	}

	native_window(
		utki::shared_ref<display_wrapper> display,
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	);

	void bind_rendering_context() override;

	void swap_frame_buffers() override;

	ruis::real get_dots_per_inch();
	ruis::real get_dots_per_pp();

	void set_mouse_cursor_visible(bool visible) override
	{
		if (visible) {
			if (!this->mouse_cursor_visible) {
				ShowCursor(TRUE);
			}
		} else {
			if (this->mouse_cursor_visible) {
				ShowCursor(FALSE);
			}
		}
		this->mouse_cursor_visible = visible;
	}

	bool is_mouse_cursor_visible() const noexcept
	{
		return this->mouse_cursor_visible;
	}

	void set_fullscreen_internal(bool enable)
	{
		if (enable) {
			// save original window size
			RECT rect;
			if (GetWindowRect(
					this->window.handle, //
					&rect
				) == 0)
			{
				throw std::runtime_error("Failed to get window rect");
			}
			this->before_fullscreen_window_rect.p.x() = rect.left;
			this->before_fullscreen_window_rect.p.y() = rect.top;
			this->before_fullscreen_window_rect.d.x() = rect.right - rect.left;
			this->before_fullscreen_window_rect.d.y() = rect.bottom - rect.top;

			// Set new window style
			SetWindowLong(
				this->window.handle, //
				GWL_STYLE,
				GetWindowLong(this->window.handle, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME)
			);
			SetWindowLong(
				this->window.handle, //
				GWL_EXSTYLE,
				GetWindowLong(this->window.handle, GWL_EXSTYLE) &
					~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
			);

			// set new window size and position
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(MonitorFromWindow(this->window.handle, MONITOR_DEFAULTTONEAREST), &mi);
			SetWindowPos(
				this->window.handle,
				nullptr,
				mi.rcMonitor.left,
				mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
			);
		} else {
			// Reset original window style
			SetWindowLong(
				this->window.handle, //
				GWL_STYLE,
				GetWindowLong(this->window.handle, GWL_STYLE) | (WS_CAPTION | WS_THICKFRAME)
			);
			SetWindowLong(
				this->window.handle, //
				GWL_EXSTYLE,
				GetWindowLong(this->window.handle, GWL_EXSTYLE) |
					(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
			);

			SetWindowPos(
				this->window.handle,
				nullptr, // no z-order change
				this->before_fullscreen_window_rect.p.x(),
				this->before_fullscreen_window_rect.p.y(),
				this->before_fullscreen_window_rect.d.x(),
				this->before_fullscreen_window_rect.d.y(),
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
			);
		}
	}
};
} // namespace
