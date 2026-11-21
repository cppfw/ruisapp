// this file is part of ruisapp application framework
//
// this file is licensed under the terms of the MIT license, which can be found in the LICENSE file
//
// SPDX-License-Identifier: MIT

// OpenGL-specific native window implementation.
// The gl_visual_info base class holds the computed GLXFBConfig + X11 visual. It is
// inherited before xorg_window so that the base class can use the visual when
// creating the X11 window (base classes are initialized before members, so the
// visual cannot be a member of the derived class).

#include <GL/glx.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "xorg_window.hxx"

namespace {

// Holds the computed GLXFBConfig + X11 visual. This is a base class of native_window
// (inherited before xorg_window) so that the visual is available when the
// xorg_window base creates the X11 window.
class gl_visual_info
{
public:
	GLXFBConfig fb_config;
	XVisualInfo* const visual_info;
	xorg_visual xvisual;

	gl_visual_info(
		display_wrapper& display, //
		const utki::version_duplet&,
		const ruisapp::window_parameters& window_params
	) :
		fb_config([&]() {
			GLXFBConfig best_fb_config = nullptr;
			std::vector<int> visual_attribs;
			visual_attribs.push_back(GLX_X_RENDERABLE);
			visual_attribs.push_back(True);
			visual_attribs.push_back(GLX_X_VISUAL_TYPE);
			visual_attribs.push_back(GLX_TRUE_COLOR);
			visual_attribs.push_back(GLX_DRAWABLE_TYPE);
			visual_attribs.push_back(GLX_WINDOW_BIT);
			visual_attribs.push_back(GLX_RENDER_TYPE);
			visual_attribs.push_back(GLX_RGBA_BIT);
			visual_attribs.push_back(GLX_DOUBLEBUFFER);
			visual_attribs.push_back(True);
			visual_attribs.push_back(GLX_RED_SIZE);
			visual_attribs.push_back(utki::byte_bits);
			visual_attribs.push_back(GLX_GREEN_SIZE);
			visual_attribs.push_back(utki::byte_bits);
			visual_attribs.push_back(GLX_BLUE_SIZE);
			visual_attribs.push_back(utki::byte_bits);
			visual_attribs.push_back(GLX_ALPHA_SIZE);
			visual_attribs.push_back(utki::byte_bits);

			if (window_params.buffers.get(ruisapp::buffer::depth)) {
				visual_attribs.push_back(GLX_DEPTH_SIZE);
				visual_attribs.push_back(utki::byte_bits * 3); // 24 bits per pixel for depth buffer
			}
			if (window_params.buffers.get(ruisapp::buffer::stencil)) {
				visual_attribs.push_back(GLX_STENCIL_SIZE);
				visual_attribs.push_back(utki::byte_bits);
			}

			visual_attribs.push_back(None);

			utki::span<GLXFBConfig> fb_configs = [&]() {
				int fbcount = 0;
				GLXFBConfig* fb_configs = glXChooseFBConfig(
					display.xorg_display.display,
					display.xorg_display.get_default_screen(),
					visual_attribs.data(),
					&fbcount
				);
				if (!fb_configs) {
					throw std::runtime_error("glXChooseFBConfig() returned empty list");
				}
				return utki::make_span(fb_configs, fbcount);
			}();

			utki::scope_exit scope_exit_fbc([&fb_configs]() {
				// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
				XFree(fb_configs.data());
			});

			GLXFBConfig worst_fb_config = nullptr;
			int best_num_samp = -1;
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
			int worst_num_samp = 999;

			for (auto fb_config : fb_configs) {
				XVisualInfo* vi = glXGetVisualFromFBConfig(
					display.xorg_display.display, //
					fb_config
				);
				if (!vi) {
					continue;
				}

				// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
				int samp_buf;
				glXGetFBConfigAttrib(
					display.xorg_display.display, //
					fb_config,
					GLX_SAMPLE_BUFFERS,
					&samp_buf
				);

				// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
				int samples;
				glXGetFBConfigAttrib(
					display.xorg_display.display, //
					fb_config,
					GLX_SAMPLES,
					&samples
				);

				if (!best_fb_config || (samp_buf && samples > best_num_samp)) {
					best_fb_config = fb_config;
					best_num_samp = samples;
				}
				if (!worst_fb_config || !samp_buf || samples < worst_num_samp) {
					worst_fb_config = fb_config;
					worst_num_samp = samples;
				}

				XFree(vi);
			}

			utki::assert(best_fb_config, SL);
			return best_fb_config;
		}()),
		visual_info([&]() {
			auto visual_info = glXGetVisualFromFBConfig(
				display.xorg_display.display, //
				this->fb_config
			);
			if (!visual_info) {
				throw std::runtime_error("glXGetVisualFromFBConfig() failed");
			}
			return visual_info;
		}()),
		xvisual([this]() {
			return xorg_visual{
				.visual = this->visual_info->visual, //
				.depth = unsigned(this->visual_info->depth)
			};
		}())
	{}

	gl_visual_info(const gl_visual_info&) = delete;
	gl_visual_info& operator=(const gl_visual_info&) = delete;

	gl_visual_info(gl_visual_info&&) = delete;
	gl_visual_info& operator=(gl_visual_info&&) = delete;

	~gl_visual_info()
	{
		XFree(this->visual_info);
	}
};

// Creates the GLX context
struct glx_context_wrapper {
	display_wrapper& display;

	enum class glx_extension {
		glx_arb_create_context,
		glx_ext_swap_control,
		glx_mesa_swap_control,

		enum_size
	};

	// glXGetProcAddressARB() will return non-null pointer even if extension is
	// not supported, so we need to explicitly check for supported extensions.
	// SOURCE:
	// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/
	const utki::flags<glx_extension> supported_extensions;

	const GLXContext context;

	glx_context_wrapper(
		display_wrapper& display, //
		XVisualInfo* visual_info,
		const utki::version_duplet& gl_version,
		GLXFBConfig fb_config,
		GLXContext shared_glx_context
	) :
		display(display),
		supported_extensions([&]() {
			utki::flags<glx_extension> supported = false;

			auto glx_extensions_string = std::string_view(glXQueryExtensionsString(
				this->display.xorg_display.display, //
				visual_info->screen
			));
			utki::log_debug([&](auto& o) {
				o << "glx_extensions_string = " << glx_extensions_string << std::endl;
			});

			auto glx_extensions = utki::split(glx_extensions_string);

			using namespace std::string_view_literals;

			if (std::ranges::find(
					glx_extensions, //
					"GLX_ARB_create_context"sv
				) != glx_extensions.end())
			{
				supported.set(glx_extension::glx_arb_create_context);
			}

			if (std::ranges::find(
					glx_extensions, //
					"GLX_EXT_swap_control"sv
				) != glx_extensions.end())
			{
				supported.set(glx_extension::glx_ext_swap_control);
			}

			if (std::ranges::find( //
					glx_extensions,
					"GLX_MESA_swap_control"sv
				) != glx_extensions.end())
			{
				supported.set(glx_extension::glx_mesa_swap_control);
			}

			return supported;
		}()),
		context([&]() {
			GLXContext gl_context = nullptr;

			if (this->supported_extensions.get(glx_extension::glx_arb_create_context)) {
				// GLX_ARB_create_context is supported

				// NOTE: glXGetProcAddressARB() is guaranteed to be present in all GLX
				// versions.
				//       glXGetProcAddress() is not guaranteed.
				// SOURCE:
				// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

				auto glx_create_context_attribs_arb = PFNGLXCREATECONTEXTATTRIBSARBPROC(glXGetProcAddressARB(
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
					reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")
				));

				if (!glx_create_context_attribs_arb) {
					throw std::runtime_error("glXCreateContextAttribsARB() not found");
				}

				auto graphics_api_version = [&ver = gl_version]() {
					if (ver.to_uint32_t() == 0) {
						// default OpenGL version is 2.0
						return utki::version_duplet{
							.major = 2, //
							.minor = 0
						};
					}
					if (ver.major < 2) {
						throw std::invalid_argument(
							utki::cat("minimal supported OpenGL version is 2.0, requested: ", ver)
						);
					}
					return ver;
				}();

				static const std::array<int, 7> context_attribs = {
					GLX_CONTEXT_MAJOR_VERSION_ARB,
					graphics_api_version.major,
					GLX_CONTEXT_MINOR_VERSION_ARB,
					graphics_api_version.minor,
					GLX_CONTEXT_PROFILE_MASK_ARB,
					// we don't need compatibility context
					GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
					None
				};

				gl_context = glx_create_context_attribs_arb(
					this->display.xorg_display.display, //
					fb_config,
					shared_glx_context,
					GL_TRUE,
					context_attribs.data()
				);
			} else {
				// GLX_ARB_create_context is not supported
				gl_context = glXCreateContext(
					this->display.xorg_display.display, //
					visual_info,
					shared_glx_context,
					GL_TRUE
				);
			}

			// sync to ensure any errors generated are processed
			XSync(
				this->display.xorg_display.display, //
				False
			);

			if (gl_context == nullptr) {
				throw std::runtime_error("glXCreateContext() failed");
			}

			return gl_context;
		}())
	{}

	glx_context_wrapper(const glx_context_wrapper&) = delete;
	glx_context_wrapper& operator=(const glx_context_wrapper&) = delete;

	glx_context_wrapper(glx_context_wrapper&&) = delete;
	glx_context_wrapper& operator=(glx_context_wrapper&&) = delete;

	~glx_context_wrapper()
	{
		if (glXGetCurrentContext() == this->context) {
			glXMakeCurrent(
				this->display.xorg_display.display, //
				None,
				nullptr
			);
		}
		glXDestroyContext(
			this->display.xorg_display.display, //
			this->context
		);
	}
};

// review: rename to opengl_native_window, do simnilar with opengles_native_window and vulkan_native_window, also in wayland implementation
class opengl_native_window : public gl_visual_info, public xorg_window
{
public:
	opengl_native_window(
		utki::shared_ref<display_wrapper> display, //
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		opengl_native_window* shared_gl_context_native_window
	) :
		gl_visual_info(
			display.get(), //
			gl_version,
			window_params
		),
		xorg_window(
			display, //
			window_params,
			shared_gl_context_native_window,
			this->xvisual
		),
		glx_context(
			this->display.get(), //
			this->visual_info,
			gl_version,
			this->fb_config,
			shared_gl_context_native_window ? shared_gl_context_native_window->glx_context.context : nullptr
		)
	{
		// if there is no any GL context current, then set this one before calling glewInit()
		if (glXGetCurrentContext() == nullptr) {
			this->bind_rendering_context();
		}
		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("GLEW initialization failed");
		}
	}

	opengl_native_window(const opengl_native_window&) = delete;
	opengl_native_window& operator=(const opengl_native_window&) = delete;

	opengl_native_window(opengl_native_window&&) = delete;
	opengl_native_window& operator=(opengl_native_window&&) = delete;

	~opengl_native_window() override = default;

	void bind_rendering_context() override
	{
		glXMakeCurrent(
			this->display.get().xorg_display.display, //
			this->window.window,
			this->glx_context.context
		);
	}

	bool is_rendering_context_bound() const noexcept override
	{
		return glXGetCurrentContext() == this->glx_context.context;
	}

	void swap_frame_buffers() override
	{
		glXSwapBuffers(
			this->display.get().xorg_display.display, //
			this->window.window
		);
		if (this->is_vsync_enabled()) {
			// With glFinish() call here it works better when VSYNC is enabled.
			// This is noticable when an app renders a mouse cursor itself,
			// the cursor moves with less latency.
			// For some reason, the effect is only present if calling the glFinish()
			// right after swapping buffers, if calling it before swapping buffers,
			// the effect is not present.
			glFinish();
		}
	}

	void set_vsync_enabled_internal(bool enable) override
	{
		utki::assert(
			[this]() {
				return this->is_rendering_context_bound();
			},
			SL
		);

		// disable v-sync via swap control extension
		if (this->glx_context.supported_extensions.get(glx_context_wrapper::glx_extension::glx_ext_swap_control)) {
			utki::log_debug([](auto& o) {
				o << "GLX_EXT_swap_control is supported\n";
			});

			auto glx_swap_interval_ext = PFNGLXSWAPINTERVALEXTPROC(glXGetProcAddressARB(
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				reinterpret_cast<const GLubyte*>("glXSwapIntervalEXT")
			));

			utki::assert(glx_swap_interval_ext, SL);

			// disable v-sync
			glx_swap_interval_ext(
				this->display.get().xorg_display.display, //
				this->window.window,
				enable ? 1 : 0 // swap interval in vsync frames
			);
		} else if (this->glx_context.supported_extensions.get(glx_context_wrapper::glx_extension::glx_mesa_swap_control
				   ))
		{
			utki::log_debug([](auto& o) {
				o << "GLX_MESA_swap_control is supported\n";
			});

			auto glx_swap_interval_mesa = PFNGLXSWAPINTERVALMESAPROC(glXGetProcAddressARB(
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				reinterpret_cast<const GLubyte*>("glXSwapIntervalMESA")
			));

			utki::assert(glx_swap_interval_mesa, SL);

			// disable v-sync
			if (glx_swap_interval_mesa(
					enable ? 1 : 0 // swap interval in vsync frames
				) != 0)
			{
				utki::logcat("WARNING: glXSwapIntervalMESA(", enable, ") failed");
			}
		} else {
			std::cout << "none of GLX_EXT_swap_control, GLX_MESA_swap_control GLX "
					  << "extensions are supported. Not disabling v-sync." << std::endl;
		}

		// sync to ensure any errors generated are processed
		XSync(
			this->display.get().xorg_display.display, //
			False
		);
	}

private:
	const glx_context_wrapper glx_context;
};

} // namespace
