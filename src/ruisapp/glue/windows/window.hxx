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
			const ruisapp::window_parameters& window_params
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
			const device_context_wrapper& device_context,
			const ruisapp::window_parameters& window_params,
			HGLRC shared_context
		);

		opengl_context_wrapper(const opengl_context_wrapper&) = delete;
		opengl_context_wrapper& operator=(const opengl_context_wrapper&) = delete;

		opengl_context_wrapper(opengl_context_wrapper&&) = delete;
		opengl_context_wrapper& operator=(opengl_context_wrapper&&) = delete;

		~opengl_context_wrapper();
	} opengl_context;
#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_display_wrapper egl_display;
	egl_config_wrapper egl_config;
	egl_surface_wrapper egl_surface;
	egl_context_wrapper egl_context;
#endif

public:
	using window_id_type = HWND;

	window_id_type get_id()const noexcept {
		return this->window.handle;
	}

	native_window(
		utki::shared_ref<display_wrapper> display,
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	);

	void bind_rendering_context()override;

	void swap_frame_buffers() override;
};
} // namespace
