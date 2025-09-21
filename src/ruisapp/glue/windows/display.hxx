#pragma once

#include <utki/windows.hpp>

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/wglext.h>
#elif defined(RUISAPP_RENDER_OPENGLES)
#	include "../egl_utils.hxx"
#endif

namespace {
class display_wrapper {
public:
struct window_class_wrapper {
	const char* const window_class_name;

	window_class_wrapper(
		const char* window_class_name,
		WNDPROC window_procedure
	);

	window_class_wrapper(const window_class_wrapper&) = delete;
	window_class_wrapper& operator=(const window_class_wrapper) = delete;

	window_class_wrapper(window_class_wrapper&&) = delete;
	window_class_wrapper& operator=(window_class_wrapper&&) = delete;

	~window_class_wrapper();
};

	window_class_wrapper dummy_window_class;
	window_class_wrapper regular_window_class;

#ifdef RUISAPP_RENDER_OPENGL
	struct wgl_procedures_wrapper {
		PFNWGLCHOOSEPIXELFORMATARBPROC wgl_choose_pixel_format_arb = nullptr;
		PFNWGLCREATECONTEXTATTRIBSARBPROC wgl_create_context_attribs_arb = nullptr;

		wgl_procedures_wrapper();

		wgl_procedures_wrapper(const wgl_procedures_wrapper&) = delete;
		wgl_procedures_wrapper& operator=(const wgl_procedures_wrapper&) = delete;

		wgl_procedures_wrapper(wgl_procedures_wrapper&&) = delete;
		wgl_procedures_wrapper& operator=(wgl_procedures_wrapper&&) = delete;

		~wgl_procedures_wrapper() = default;
	}wgl_procedures;
#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_display_wrapper egl_display;
#endif

	display_wrapper();
};
}
