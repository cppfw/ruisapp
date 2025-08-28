#pragma once

#include <string_view>

#include <EGL/egl.h>

namespace {
std::string_view egl_error_to_string(EGLint err)
{
	using namespace std::string_view_literals;
	switch (err) {
		case EGL_SUCCESS:
			return "EGL_SUCCESS"sv;
		case EGL_NOT_INITIALIZED:
			return "EGL_NOT_INITIALIZED"sv;
		case EGL_BAD_ACCESS:
			return "EGL_BAD_ACCESS"sv;
		case EGL_BAD_ALLOC:
			return "EGL_BAD_ALLOC"sv;
		case EGL_BAD_ATTRIBUTE:
			return "EGL_BAD_ATTRIBUTE"sv;
		case EGL_BAD_CONTEXT:
			return "EGL_BAD_CONTEXT"sv;
		case EGL_BAD_CONFIG:
			return "EGL_BAD_CONFIG"sv;
		case EGL_BAD_CURRENT_SURFACE:
			return "EGL_BAD_CURRENT_SURFACE"sv;
		case EGL_BAD_DISPLAY:
			return "EGL_BAD_DISPLAY"sv;
		case EGL_BAD_SURFACE:
			return "EGL_BAD_SURFACE"sv;
		case EGL_BAD_MATCH:
			return "EGL_BAD_MATCH"sv;
		case EGL_BAD_PARAMETER:
			return "EGL_BAD_PARAMETER"sv;
		case EGL_BAD_NATIVE_PIXMAP:
			return "EGL_BAD_NATIVE_PIXMAP"sv;
		case EGL_BAD_NATIVE_WINDOW:
			return "EGL_BAD_NATIVE_WINDOW"sv;
		case EGL_CONTEXT_LOST:
			return "EGL_CONTEXT_LOST"sv;
		default:
			return "Unknown EGL error"sv;
	}
}
} // namespace
