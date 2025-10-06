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

#pragma once

#include <stdexcept>
#include <string_view>

#include <EGL/egl.h>
#include <utki/string.hpp>
#include <utki/version.hpp>

#include "../window.hpp"

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

namespace {
namespace egl {
enum class extension {
	khr_surfaceless_context,

	enum_size
};
} // namespace egl
} // namespace

namespace {
struct egl_display_wrapper {
public:
	const EGLDisplay display;

	const utki::version_duplet egl_version;

	const utki::flags<egl::extension> extensions;

private:
	utki::flags<egl::extension> get_egl_extensions()
	{
		utki::flags<egl::extension> exts{false};

		const char* exts_str = eglQueryString(this->display, EGL_EXTENSIONS);
		utki::assert(
			exts_str, //
			[](auto& o) {
				o << "eglQueryString(EGL_EXTENSIONS) failed, error: " << egl_error_to_string(eglGetError());
			},
			SL
		);
		utki::logcat_debug("EGL extensions string = ", exts_str, '\n');

		auto ext_strs = utki::split(std::string_view(exts_str));

		utki::logcat_debug("EGL extensions detected:", '\n');
		for (auto& e : ext_strs) {
			using namespace std::string_view_literals;

			if (e == "EGL_KHR_surfaceless_context"sv) {
				exts.set(egl::extension::khr_surfaceless_context);
				utki::logcat_debug("  EGL_KHR_surfaceless_context", '\n');
			}
		}

		return exts;
	}

public:
	egl_display_wrapper(EGLNativeDisplayType display_id = EGL_DEFAULT_DISPLAY) :
		display([&]() {
			auto d = eglGetDisplay(display_id);
			if (d == EGL_NO_DISPLAY) {
				throw std::runtime_error(utki::cat(
					"eglGetDisplay(): failed, error: ", //
					egl_error_to_string(eglGetError())
				));
			}

			return d;
		}()),
		egl_version([&]() {
			EGLint major = 0;
			EGLint minor = 0;

			if (eglInitialize(
					this->display, //
					&major,
					&minor
				) == EGL_FALSE)
			{
				eglTerminate(this->display);
				throw std::runtime_error("eglInitialize() failed");
			}
			return utki::version_duplet{
				uint16_t(major), //
				uint16_t(minor)
			};
		}()),
		extensions(this->get_egl_extensions())
	{
		utki::logcat_debug("EGL version = ", this->egl_version, '\n');

		try {
			if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
				throw std::runtime_error("eglBindApi() failed");
			}
		} catch (...) {
			eglTerminate(this->display);
			throw;
		}
	}

	egl_display_wrapper(const egl_display_wrapper&) = delete;
	egl_display_wrapper& operator=(const egl_display_wrapper&) = delete;

	egl_display_wrapper(egl_display_wrapper&&) = delete;
	egl_display_wrapper& operator=(egl_display_wrapper&&) = delete;

	~egl_display_wrapper()
	{
		eglTerminate(this->display);
	}
};
} // namespace

namespace {
struct egl_config_wrapper {
	EGLConfig config;

	egl_config_wrapper(
		egl_display_wrapper& egl_display,
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params
	) :
		config([&]() {
			EGLConfig egl_config = nullptr;

			// Here specify the attributes of the desired configuration.
			// Below, we select an EGLConfig with at least 8 bits per color
			// component compatible with on-screen windows.
			const std::array<EGLint, 15> attribs = {
				EGL_SURFACE_TYPE,
				EGL_WINDOW_BIT,
				EGL_RENDERABLE_TYPE,
				// We cannot set bits for all OpenGL ES versions because on platforms which do not
				// support later versions the matching config will not be found by eglChooseConfig().
				// So, set bits according to requested OpenGL ES version.
				[&ver = gl_version]() {
					EGLint ret = EGL_OPENGL_ES2_BIT; // OpenGL ES 2 is the minimum
					if (ver.major >= 3) {
						ret |= EGL_OPENGL_ES3_BIT;
					}
					return ret;
				}(),
				EGL_BLUE_SIZE,
				8,
				EGL_GREEN_SIZE,
				8,
				EGL_RED_SIZE,
				8,
				EGL_DEPTH_SIZE,
				window_params.buffers.get(ruisapp::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
				EGL_STENCIL_SIZE,
				window_params.buffers.get(ruisapp::buffer::stencil) ? utki::byte_bits : 0,
				EGL_NONE
			};

			// Here, the application chooses the configuration it desires. In this
			// sample, we have a very simplified selection process, where we pick
			// the first EGLConfig that matches our criteria.
			EGLint num_configs = 0;
			eglChooseConfig(
				egl_display.display, //
				attribs.data(),
				&egl_config,
				1,
				&num_configs
			);
			if (num_configs <= 0) {
				throw std::runtime_error("eglChooseConfig() failed, no matching config found");
			}

			utki::assert(egl_config, SL);

			return egl_config;
		}())
	{}

	egl_config_wrapper(const egl_config_wrapper&) = delete;
	egl_config_wrapper& operator=(const egl_config_wrapper&) = delete;

	egl_config_wrapper(egl_config_wrapper&&) = delete;
	egl_config_wrapper& operator=(egl_config_wrapper&&) = delete;

	// no need to free the egl config
	~egl_config_wrapper() = default;
};
} // namespace

namespace {
struct egl_surface_wrapper {
	egl_display_wrapper& egl_display;

	const EGLSurface surface;

	egl_surface_wrapper(
		egl_display_wrapper& egl_display, //
		const egl_config_wrapper& egl_config,
		EGLNativeWindowType window
	) :
		egl_display(egl_display),
		surface([&]() {
			auto s = eglCreateWindowSurface(
				this->egl_display.display, //
				egl_config.config,
				window,
				nullptr
			);
			if (s == EGL_NO_SURFACE) {
				throw std::runtime_error(utki::cat(
					"eglCreateWindowSurface() failed, error: ", //
					egl_error_to_string(eglGetError())
				));
			}
			return s;
		}())
	{}

	egl_surface_wrapper(const egl_surface_wrapper&) = delete;
	egl_surface_wrapper& operator=(const egl_surface_wrapper&) = delete;

	egl_surface_wrapper(egl_surface_wrapper&&) = delete;
	egl_surface_wrapper& operator=(egl_surface_wrapper&&) = delete;

	~egl_surface_wrapper()
	{
		eglDestroySurface(
			this->egl_display.display, //
			this->surface
		);
	}

	void swap_frame_buffers()
	{
		eglSwapBuffers(
			this->egl_display.display, //
			this->surface
		);
	}

	r4::vector2<unsigned> get_dims()
	{
		EGLint width = 0;
		eglQuerySurface(
			this->egl_display.display, //
			this->surface,
			EGL_WIDTH,
			&width
		);

		EGLint height = 0;
		eglQuerySurface(
			this->egl_display.display, //
			this->surface,
			EGL_HEIGHT,
			&height
		);
		return r4::vector2<unsigned>(width, height);
	}
};
} // namespace

namespace {
struct egl_pbuffer_surface_wrapper {
	egl_display_wrapper& egl_display;

	const EGLSurface surface;

	egl_pbuffer_surface_wrapper(
		egl_display_wrapper& egl_display, //
		const egl_config_wrapper& egl_config
	) :
		egl_display(egl_display),
		surface([&]() {
			const std::array<EGLint, 1> attribs = {EGL_NONE};

			auto s = eglCreatePbufferSurface(
				this->egl_display.display, //
				egl_config.config,
				attribs.data()
			);
			if (s == EGL_NO_SURFACE) {
				throw std::runtime_error(utki::cat(
					"eglCreatePbufferSurface() failed, error: ", //
					egl_error_to_string(eglGetError())
				));
			}
			return s;
		}())
	{}

	egl_pbuffer_surface_wrapper(const egl_pbuffer_surface_wrapper&) = delete;
	egl_pbuffer_surface_wrapper& operator=(const egl_pbuffer_surface_wrapper&) = delete;

	egl_pbuffer_surface_wrapper(egl_pbuffer_surface_wrapper&&) = delete;
	egl_pbuffer_surface_wrapper& operator=(egl_pbuffer_surface_wrapper&&) = delete;

	~egl_pbuffer_surface_wrapper()
	{
		eglDestroySurface(
			this->egl_display.display, //
			this->surface
		);
	}
};
} // namespace

namespace {
struct egl_context_wrapper {
	egl_display_wrapper& egl_display;

	std::unique_ptr<egl_pbuffer_surface_wrapper> pbuffer_surface;

	const EGLContext context;

	egl_context_wrapper(
		egl_display_wrapper& egl_display, //
		const utki::version_duplet& gl_version,
		const egl_config_wrapper& egl_config,
		EGLContext shared_context = EGL_NO_CONTEXT
	) :
		egl_display(egl_display),
		context([&]() {
			auto graphics_api_version = [&ver = gl_version]() {
				if (ver.to_uint32_t() == 0) {
					// default OpenGL ES version is 2.0
					return utki::version_duplet{
						.major = 2, //
						.minor = 0
					};
				}

				if (ver.major < 2) {
					throw std::invalid_argument(
						utki::cat("minimal supported OpenGL ES version is 2.0, requested: ", ver)
					);
				}
				return ver;
			}();

			constexpr auto attrs_array_size = 5;
			std::array<EGLint, attrs_array_size> context_attrs = {
				EGL_CONTEXT_MAJOR_VERSION,
				graphics_api_version.major,
				EGL_CONTEXT_MINOR_VERSION,
				graphics_api_version.minor,
				EGL_NONE
			};

			auto egl_context = eglCreateContext(
				this->egl_display.display, //
				egl_config.config,
				shared_context,
				context_attrs.data()
			);
			if (egl_context == EGL_NO_CONTEXT) {
				auto message = utki::cat(
					"eglCreateContext() failed, error: ", //
					egl_error_to_string(eglGetError())
				);
				throw std::runtime_error(std::move(message));
			}
			return egl_context;
		}())
	{}

	egl_context_wrapper(const egl_context_wrapper&) = delete;
	egl_context_wrapper& operator=(const egl_context_wrapper&) = delete;

	egl_context_wrapper(egl_context_wrapper&&) = delete;
	egl_context_wrapper& operator=(egl_context_wrapper&&) = delete;

	~egl_context_wrapper()
	{
		if (eglGetCurrentContext() == this->context) {
			eglMakeCurrent(
				this->egl_display.display, //
				EGL_NO_SURFACE,
				EGL_NO_SURFACE,
				EGL_NO_CONTEXT
			);
		}
		eglDestroyContext(
			this->egl_display.display, //
			this->context
		);
	}

	void set_vsync_enabled(bool enabled)
	{
		utki::assert(
			eglGetCurrentContext() == this->context,
			[](auto& o) {
				o << "egl_context_wrapper::disable_vsync(): the EGL context is not current";
			},
			SL
		);

		if (eglSwapInterval(
				this->egl_display.display, //
				enabled ? 1 : 0 // number of vsync frames before framebuffer buffer swap
			) != EGL_TRUE)
		{
			throw std::runtime_error("egl_context_wrapper::disable_vsync(): eglSwapInterval(0) failed");
		}
	}
};
} // namespace
