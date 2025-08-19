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

#include <array>
#include <string_view>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <gdk/gdk.h>
#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <papki/fs_file.hpp>
#include <utki/string.hpp>
#include <utki/unicode.hpp>

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glew.h>
#	include <GL/glx.h>
#	include <ruis/render/opengl/context.hpp>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <GLES2/gl2.h>
#	ifdef RUISAPP_RASPBERRYPI
#		include <bcm_host.h>
#	endif

#	include <ruis/render/opengles/context.hpp>

#else
#	error "Unknown graphics API"
#endif

#include "../../application.hpp"
#include "../friend_accessors.cxx" // NOLINT(bugprone-suspicious-include)
#include "../unix_common.cxx" // NOLINT(bugprone-suspicious-include)

using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
const std::map<ruis::mouse_cursor, unsigned> x_cursor_map = {
	{			   ruis::mouse_cursor::arrow,            XC_left_ptr},
	{    ruis::mouse_cursor::left_right_arrow,   XC_sb_h_double_arrow},
	{	   ruis::mouse_cursor::up_down_arrow,   XC_sb_v_double_arrow},
	{ruis::mouse_cursor::all_directions_arrow,               XC_fleur},
	{		   ruis::mouse_cursor::left_side,           XC_left_side},
	{		  ruis::mouse_cursor::right_side,          XC_right_side},
	{			ruis::mouse_cursor::top_side,            XC_top_side},
	{		 ruis::mouse_cursor::bottom_side,         XC_bottom_side},
	{	 ruis::mouse_cursor::top_left_corner,     XC_top_left_corner},
	{    ruis::mouse_cursor::top_right_corner,    XC_top_right_corner},
	{  ruis::mouse_cursor::bottom_left_corner,  XC_bottom_left_corner},
	{ ruis::mouse_cursor::bottom_right_corner, XC_bottom_right_corner},
	{		ruis::mouse_cursor::index_finger,               XC_hand2},
	{				ruis::mouse_cursor::grab,               XC_hand1},
	{			   ruis::mouse_cursor::caret,               XC_xterm}
};
} // namespace

namespace {
struct window_wrapper : public utki::destructable {
	struct display_wrapper {
		Display* display;

		display_wrapper() :
			display([]() {
				auto display = XOpenDisplay(nullptr);
				if (!display) {
					throw std::runtime_error("XOpenDisplay() failed");
				}
				return display;
			}())
		{}

		display_wrapper(const display_wrapper&) = delete;
		display_wrapper& operator=(const display_wrapper&) = delete;

		display_wrapper(display_wrapper&&) = delete;
		display_wrapper& operator=(display_wrapper&&) = delete;

		~display_wrapper()
		{
			XCloseDisplay(this->display);
		}
	} display;

	Colormap color_map;
	::Window window;

	ruis::real scale_factor = 1;

#ifdef RUISAPP_RENDER_OPENGL
	GLXContext gl_context;
#elif defined(RUISAPP_RENDER_OPENGLES)
#	ifdef RUISAPP_RASPBERRYPI
	EGL_DISPMANX_WINDOW_T rpi_native_window{};
	DISPMANX_DISPLAY_HANDLE_T rpi_dispman_display;
	DISPMANX_UPDATE_HANDLE_T rpi_dispman_update;
	DISPMANX_ELEMENT_HANDLE_T rpi_dispman_element;
#	endif
	EGLDisplay egl_display;
	EGLSurface egl_surface;
	EGLContext egl_context;
#else
#	error "Unknown graphics API"
#endif
	struct cursor_wrapper {
		// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
		window_wrapper& owner;
		Cursor cursor;

		cursor_wrapper(window_wrapper& owner, ruis::mouse_cursor c) :
			owner(owner)
		{
			if (c == ruis::mouse_cursor::none) {
				std::array<char, 1> data = {0};

				Pixmap blank =
					XCreateBitmapFromData(this->owner.display.display, this->owner.window, data.data(), 1, 1);
				if (blank == None) {
					throw std::runtime_error(
						"application::XEmptyMouseCursor::XEmptyMouseCursor(): could not "
						"create bitmap"
					);
				}
				utki::scope_exit scope_exit([this, &blank]() {
					XFreePixmap(this->owner.display.display, blank);
				});

				XColor dummy;
				this->cursor = XCreatePixmapCursor(this->owner.display.display, blank, blank, &dummy, &dummy, 0, 0);
			} else {
				this->cursor = XCreateFontCursor(this->owner.display.display, x_cursor_map.at(c));
			}
		}

		cursor_wrapper(const cursor_wrapper&) = delete;
		cursor_wrapper& operator=(const cursor_wrapper&) = delete;

		cursor_wrapper(cursor_wrapper&&) = delete;
		cursor_wrapper& operator=(cursor_wrapper&&) = delete;

		~cursor_wrapper()
		{
			XFreeCursor(this->owner.display.display, this->cursor);
		}
	};

	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	cursor_wrapper* cur_cursor = nullptr;
	bool cursor_visible = true;
	std::map<ruis::mouse_cursor, std::unique_ptr<cursor_wrapper>> cursors;

	void apply_cursor(cursor_wrapper& c)
	{
		XDefineCursor(this->display.display, this->window, c.cursor);
	}

	cursor_wrapper* get_cursor(ruis::mouse_cursor c)
	{
		auto i = this->cursors.find(c);
		if (i == this->cursors.end()) {
			i = this->cursors.insert(std::make_pair(c, std::make_unique<cursor_wrapper>(*this, c))).first;
		}
		return i->second.get();
	}

	void set_cursor(ruis::mouse_cursor c)
	{
		this->cur_cursor = this->get_cursor(c);

		if (this->cursor_visible) {
			this->apply_cursor(*this->cur_cursor);
		}
	}

	void set_cursor_visible(bool visible)
	{
		this->cursor_visible = visible;
		if (visible) {
			if (this->cur_cursor) {
				this->apply_cursor(*this->cur_cursor);
			} else {
				XUndefineCursor(this->display.display, this->window);
			}
		} else {
			this->apply_cursor(*this->get_cursor(ruis::mouse_cursor::none));
		}
	}

	XIM input_method;
	XIC input_context;

	nitki::queue ui_queue;

	std::atomic_bool quit_flag = false;

	window_wrapper(const window_parameters& wp)
	{
		// set scale factor
		{
			gdk_init(nullptr, nullptr);

			// GDK-4 version commented out because GDK-4 is not available in Debian 11

			// auto display_name = DisplayString(ww.display.display);
			// std::cout << "display name = " << display_name << std::endl;
			// auto disp = gdk_display_open(display_name);
			// utki::assert(disp, SL);
			// std::cout << "gdk display name = " << gdk_display_get_name(disp) << std::endl;
			// auto surf = gdk_surface_new_toplevel (disp);
			// utki::assert(surf, SL);
			// auto mon = gdk_display_get_monitor_at_surface (disp, surf);
			// utki::assert(mon, SL);
			// int sf = gdk_monitor_get_scale_factor(mon);

			// GDK-3 version
			int sf = gdk_window_get_scale_factor(gdk_get_default_root_window());
			this->scale_factor = ruis::real(sf);

			std::cout << "display scale factor = " << this->scale_factor << std::endl;
		}

#ifdef RUISAPP_RENDER_OPENGL
		auto graphics_api_version = [&ver = wp.graphics_api_version]() {
			if (ver.to_uint32_t() == 0) {
				// default OpenGL version is 2.0
				return utki::version_duplet{
					.major = 2, //
					.minor = 0
				};
			}
			return ver;
		}();

		{
			int glx_ver_major = 0;
			int glx_ver_minor = 0;
			if (!glXQueryVersion(this->display.display, &glx_ver_major, &glx_ver_minor)) {
				throw std::runtime_error("glXQueryVersion() failed");
			}

			// we need the following:
			// - glXQueryExtensionsString(), availabale starting from GLX version 1.1
			// - FBConfigs, availabale starting from GLX version 1.3
			// minimum GLX version needed is 1.3
			if (glx_ver_major < 1 || (glx_ver_major == 1 && glx_ver_minor < 3)) {
				throw std::runtime_error("GLX version 1.3 or above is required");
			}
		}

		GLXFBConfig best_fb_config = nullptr;
		{
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

			if (wp.buffers.get(ruisapp::buffer::depth)) {
				visual_attribs.push_back(GLX_DEPTH_SIZE);
				visual_attribs.push_back(utki::byte_bits * 3); // 24 bits per pixel for depth buffer
			}
			if (wp.buffers.get(ruisapp::buffer::stencil)) {
				visual_attribs.push_back(GLX_STENCIL_SIZE);
				visual_attribs.push_back(utki::byte_bits);
			}

			visual_attribs.push_back(None);

			utki::span<GLXFBConfig> fb_configs = [&]() {
				int fbcount = 0;
				GLXFBConfig* fb_configs = glXChooseFBConfig(
					this->display.display,
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
					DefaultScreen(this->display.display),
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
				// for (size_t i = 0; i < fb_config.size(); ++i) {
				XVisualInfo* vi = glXGetVisualFromFBConfig(this->display.display, fb_config);
				if (!vi) {
					continue;
				}

				// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
				int samp_buf;
				glXGetFBConfigAttrib(this->display.display, fb_config, GLX_SAMPLE_BUFFERS, &samp_buf);

				// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
				int samples;
				glXGetFBConfigAttrib(this->display.display, fb_config, GLX_SAMPLES, &samples);

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
		}
		ASSERT(best_fb_config)
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

		// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
		this->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if (this->egl_display == EGL_NO_DISPLAY) {
			throw std::runtime_error("eglGetDisplay(): failed, no matching display connection found");
		}

		utki::scope_exit scope_exit_egl_display([this]() {
			eglTerminate(this->egl_display);
		});

		if (eglInitialize(this->egl_display, nullptr, nullptr) == EGL_FALSE) {
			throw std::runtime_error("eglInitialize() failed");
		}

		EGLConfig egl_config = nullptr;
		{
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
				[&ver = wp.graphics_api_version]() {
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
				wp.buffers.get(ruisapp::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
				EGL_STENCIL_SIZE,
				wp.buffers.get(ruisapp::buffer::stencil) ? utki::byte_bits : 0,
				EGL_NONE
			};

			// Here, the application chooses the configuration it desires. In this
			// sample, we have a very simplified selection process, where we pick
			// the first EGLConfig that matches our criteria.
			EGLint num_configs = 0;
			eglChooseConfig(this->egl_display, attribs.data(), &egl_config, 1, &num_configs);
			if (num_configs <= 0) {
				throw std::runtime_error("eglChooseConfig() failed, no matching config found");
			}
		}

		if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
			throw std::runtime_error("eglBindApi() failed");
		}
#else
#	error "Unknown graphics API"
#endif

		XVisualInfo* visual_info = nullptr;
#ifdef RUISAPP_RENDER_OPENGL
		visual_info = glXGetVisualFromFBConfig(this->display.display, best_fb_config);
		if (!visual_info) {
			throw std::runtime_error("glXGetVisualFromFBConfig() failed");
		}
#elif defined(RUISAPP_RENDER_OPENGLES)
#	ifdef RUISAPP_RASPBERRYPI
		{
			// the variable is initialied via output argument, so no need to
			// initialize it here
			// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
			int num_visuals;
			XVisualInfo vis_template;
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
			vis_template.screen = DefaultScreen(this->display.display); // LCD
			visual_info = XGetVisualInfo(this->display.display, VisualScreenMask, &vis_template, &num_visuals);
			if (!visual_info) {
				throw std::runtime_error("XGetVisualInfo() failed");
			}
		}
#	else
		{
			EGLint vid = 0;

			if (!eglGetConfigAttrib(this->egl_display, egl_config, EGL_NATIVE_VISUAL_ID, &vid)) {
				throw std::runtime_error("eglGetConfigAttrib() failed");
			}

			int num_visuals = 0;
			XVisualInfo vis_template;
			vis_template.visualid = vid;
			visual_info = XGetVisualInfo(this->display.display, VisualIDMask, &vis_template, &num_visuals);
			if (!visual_info) {
				throw std::runtime_error("XGetVisualInfo() failed");
			}
		}
#	endif
#else
#	error "Unknown graphics API"
#endif
		utki::scope_exit scope_exit_visual_info([visual_info]() {
			XFree(visual_info);
		});

		this->color_map = XCreateColormap(
			this->display.display,
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			RootWindow(this->display.display, visual_info->screen),
			visual_info->visual,
			AllocNone
		);
		if (this->color_map == None) {
			// TODO: use XSetErrorHandler() to get error code
			throw std::runtime_error("XCreateColormap(): failed");
		}
		utki::scope_exit scope_exit_color_map([this]() {
			XFreeColormap(this->display.display, this->color_map);
		});

		{
			XSetWindowAttributes attr;
			attr.colormap = this->color_map;
			attr.border_pixel = 0;
			attr.background_pixmap = None;
			attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
				PointerMotionMask | ButtonMotionMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask;
			unsigned long fields = CWBorderPixel | CWColormap | CWEventMask;

			auto dims = (this->scale_factor * wp.dims.to<ruis::real>()).to<unsigned>();

			this->window = XCreateWindow(
				this->display.display,
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
				RootWindow(this->display.display, visual_info->screen),
				0,
				0,
				dims.x(),
				dims.y(),
				0,
				visual_info->depth,
				InputOutput,
				visual_info->visual,
				fields,
				&attr
			);
		}
		if (!this->window) {
			throw std::runtime_error("Failed to create window");
		}
		utki::scope_exit scope_exit_window([this]() {
			XDestroyWindow(this->display.display, this->window);
		});

		{ // we want to handle WM_DELETE_WINDOW event to know when window is closed
			Atom a = XInternAtom(this->display.display, "WM_DELETE_WINDOW", True);
			XSetWMProtocols(this->display.display, this->window, &a, 1);
		}

		XMapWindow(this->display.display, this->window);

		XFlush(this->display.display);

		// set window title
		XStoreName(
			this->display.display, //
			this->window,
			wp.title.c_str()
		);

		//====================
		// create GLX context

#ifdef RUISAPP_RENDER_OPENGL
		// glXGetProcAddressARB() will retutn non-null pointer even if extension is
		// not supported, so we need to explicitly check for supported extensions.
		// SOURCE:
		// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

		auto glx_extensions_string =
			std::string_view(glXQueryExtensionsString(this->display.display, visual_info->screen));
		utki::log_debug([&](auto& o) {
			o << "glx_extensions_string = " << glx_extensions_string << std::endl;
		});

		auto glx_extensions = utki::split(glx_extensions_string);

		if (std::find(glx_extensions.begin(), glx_extensions.end(), "GLX_ARB_create_context") == glx_extensions.end()) {
			// GLX_ARB_create_context is not supported
			this->gl_context = glXCreateContext(this->display.display, visual_info, nullptr, GL_TRUE);
		} else {
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
				// this should not happen since we checked extension presence, and
				// anyway, glXGetProcAddressARB() never returns nullptr according to
				// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/
				// so, this check for null is just in case future version of GLX may
				// return null
				throw std::runtime_error("glXCreateContextAttribsARB() not found");
			}

			const auto& ver = graphics_api_version;

			static const std::array<int, 7> context_attribs = {
				GLX_CONTEXT_MAJOR_VERSION_ARB,
				ver.major,
				GLX_CONTEXT_MINOR_VERSION_ARB,
				ver.minor,
				GLX_CONTEXT_PROFILE_MASK_ARB,
				// we don't need compatibility context
				GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
				None
			};

			this->gl_context = glx_create_context_attribs_arb(
				this->display.display,
				best_fb_config,
				nullptr,
				GL_TRUE,
				context_attribs.data()
			);
		}

		// sync to ensure any errors generated are processed
		XSync(this->display.display, False);

		if (this->gl_context == nullptr) {
			throw std::runtime_error("glXCreateContext() failed");
		}
		utki::scope_exit scope_exit_gl_context([this]() {
			glXMakeCurrent(this->display.display, None, nullptr);
			glXDestroyContext(this->display.display, this->gl_context);
		});

		glXMakeCurrent(this->display.display, this->window, this->gl_context);

		// disable v-sync via swap control extension

		if (std::find(glx_extensions.begin(), glx_extensions.end(), "GLX_EXT_swap_control") != glx_extensions.end()) {
			utki::log_debug([](auto& o) {
				o << "GLX_EXT_swap_control is supported\n";
			});

			auto glx_swap_interval_ext = PFNGLXSWAPINTERVALEXTPROC(glXGetProcAddressARB(
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				reinterpret_cast<const GLubyte*>("glXSwapIntervalEXT")
			));

			ASSERT(glx_swap_interval_ext)

			// disable v-sync
			glx_swap_interval_ext(this->display.display, this->window, 0);
		} else if ( //
			std::find( //
					   glx_extensions.begin(),
					   glx_extensions.end(),
					   "GLX_MESA_swap_control"
				   ) != glx_extensions.end()
		)
		{
			utki::log_debug([](auto& o) {
				o << "GLX_MESA_swap_control is supported\n";
			});

			auto glx_swap_interval_mesa = PFNGLXSWAPINTERVALMESAPROC(glXGetProcAddressARB(
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				reinterpret_cast<const GLubyte*>("glXSwapIntervalMESA")
			));

			ASSERT(glx_swap_interval_mesa)

			// disable v-sync
			if (glx_swap_interval_mesa(0) != 0) {
				throw std::runtime_error("glXSwapIntervalMESA() failed");
			}
		} else {
			std::cout << "none of GLX_EXT_swap_control, GLX_MESA_swap_control GLX "
						 "extensions are supported"
					  << std::endl;
		}

		// sync to ensure any errors generated are processed
		XSync(this->display.display, False);

		//=============
		// init OpenGL

		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("GLEW initialization failed");
		}
#elif defined(RUISAPP_RENDER_OPENGLES)

#	ifdef RUISAPP_RASPBERRYPI
		{
			bcm_host_init();

			VC_RECT_T dst_rect, src_rect;

			// the variables are initialized via output argument, so no need to
			// initialize it here
			// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
			uint32_t display_width, display_height;

			// create an EGL window surface, passing context width/height
			if (graphics_get_display_size(
					0, // LCD
					&display_width,
					&display_height
				) < 0)
			{
				throw std::runtime_error("graphics_get_display_size() failed");
			}

			dst_rect.x = 0;
			dst_rect.y = 0;
			dst_rect.width = display_width;
			dst_rect.height = display_height;

			src_rect.x = 0;
			src_rect.y = 0;
			src_rect.width = display_width << (utki::byte_bits * 2);
			src_rect.height = display_height << (utki::byte_bits * 2);

			this->rpi_dispman_display = vc_dispmanx_display_open(0); // 0 = LCD
			this->rpi_dispman_update = vc_dispmanx_update_start(0);

			this->rpi_dispman_element = vc_dispmanx_element_add(
				this->rpi_dispman_update,
				this->rpi_dispman_display,
				0, // layer
				&dst_rect,
				0, // src
				&src_rect,
				DISPMANX_PROTECTION_NONE,
				nullptr, // alpha
				nullptr, // clamp
				DISPMANX_NO_ROTATE // transform
			);

			this->rpi_native_window.element = this->rpi_dispman_element;
			this->rpi_native_window.width = display_width;
			this->rpi_native_window.height = display_height;
			vc_dispmanx_update_submit_sync(this->rpi_dispman_update);
		}
#	endif

		this->egl_surface = eglCreateWindowSurface(
			this->egl_display,
			egl_config,
#	ifdef RUISAPP_RASPBERRYPI
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			reinterpret_cast<EGLNativeWindowType>(&this->rpi_native_window),
#	else
			this->window,
#	endif
			nullptr
		);
		if (this->egl_surface == EGL_NO_SURFACE) {
			throw std::runtime_error("eglCreateWindowSurface() failed");
		}
		utki::scope_exit scope_exit_egl_surface([this]() {
			eglDestroySurface(this->egl_display, this->egl_surface);
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

			this->egl_context = eglCreateContext(this->egl_display, egl_config, EGL_NO_CONTEXT, context_attrs.data());
			if (this->egl_context == EGL_NO_CONTEXT) {
				throw std::runtime_error("eglCreateContext() failed");
			}
		}

		if (eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context) == EGL_FALSE) {
			eglDestroyContext(this->egl_display, this->egl_context);
			throw std::runtime_error("eglMakeCurrent() failed");
		}
		utki::scope_exit scope_exit_egl_context([this]() {
			eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroyContext(this->egl_display, this->egl_context);
		});

		// disable v-sync
		if (eglSwapInterval(this->egl_display, 0) != EGL_TRUE) {
			throw std::runtime_error("eglSwapInterval() failed");
		}
#else
#	error "Unknown graphics API"
#endif

		// initialize input method

		this->input_method = XOpenIM(this->display.display, nullptr, nullptr, nullptr);
		if (this->input_method == nullptr) {
			throw std::runtime_error("XOpenIM() failed");
		}
		utki::scope_exit scope_exit_input_method([this]() {
			XCloseIM(this->input_method);
		});

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		this->input_context = XCreateIC(
			this->input_method,
			XNClientWindow,
			this->window,
			XNFocusWindow,
			this->window,
			XNInputStyle,
			XIMPreeditNothing | XIMStatusNothing,
			nullptr
		);
		if (this->input_context == nullptr) {
			throw std::runtime_error("XCreateIC() failed");
		}
		utki::scope_exit scope_exit_input_context([this]() {
			XUnsetICFocus(this->input_context);
			XDestroyIC(this->input_context);
		});

		scope_exit_input_context.release();
		scope_exit_input_method.release();
		scope_exit_window.release();
		scope_exit_color_map.release();
#ifdef RUISAPP_RENDER_OPENGL
		scope_exit_gl_context.release();
#elif defined(RUISAPP_RENDER_OPENGLES)
		scope_exit_egl_display.release();
		scope_exit_egl_surface.release();
		scope_exit_egl_context.release();
#else
#	error "Unknown graphics API"
#endif
	}

	window_wrapper(const window_wrapper&) = delete;
	window_wrapper& operator=(const window_wrapper&) = delete;

	window_wrapper(window_wrapper&&) = delete;
	window_wrapper& operator=(window_wrapper&&) = delete;

	~window_wrapper() override
	{
		XUnsetICFocus(this->input_context);
		XDestroyIC(this->input_context);

		XCloseIM(this->input_method);

#ifdef RUISAPP_RENDER_OPENGL
		glXMakeCurrent(this->display.display, None, nullptr);
		glXDestroyContext(this->display.display, this->gl_context);
#elif defined(RUISAPP_RENDER_OPENGLES)
		eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(this->egl_display, this->egl_context);
		eglDestroySurface(this->egl_display, this->egl_surface);
#else
#	error "Unknown graphics API"
#endif

		XDestroyWindow(this->display.display, this->window);
		XFreeColormap(this->display.display, this->color_map);

#ifdef RUISAPP_RENDER_OPENGLES
		eglTerminate(this->egl_display);
#endif
	}

	ruis::real get_dots_per_inch()
	{
		int src_num = 0;

		constexpr auto mm_per_cm = 10;

		ruis::real value =
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			((ruis::real(DisplayWidth(this->display.display, src_num))
			  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			  / (ruis::real(DisplayWidthMM(this->display.display, src_num)) / ruis::real(mm_per_cm)))
			 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			 +
			 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			 (ruis::real(DisplayHeight(this->display.display, src_num))
			  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			  / (ruis::real(DisplayHeightMM(this->display.display, src_num)) / ruis::real(mm_per_cm)))) /
			2;
		value *= ruis::real(utki::cm_per_inch);
		return value;
	}

	ruis::real get_dots_per_pp()
	{
		// TODO: use scale factor only for desktop monitors
		if (this->scale_factor != ruis::real(1)) {
			return this->scale_factor;
		}

		int src_num = 0;
		r4::vector2<unsigned> resolution(
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayWidth(this->display.display, src_num),
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayHeight(this->display.display, src_num)
		);
		r4::vector2<unsigned> screen_size_mm(
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayWidthMM(this->display.display, src_num),
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
			DisplayHeightMM(this->display.display, src_num)
		);

		return application::get_pixels_per_pp(resolution, screen_size_mm);
	}
};

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
{
	ASSERT(dynamic_cast<window_wrapper*>(pimpl.get()))
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
	return static_cast<window_wrapper&>(*pimpl);
}

window_wrapper& get_impl(application& app)
{
	return get_impl(get_window_pimpl(app));
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
				utki::make_shared<ruis::render::renderer>(
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
				[this](std::function<void()> proc) {
					get_impl(get_window_pimpl(*this)).ui_queue.push_back(std::move(proc));
				},
			.set_mouse_cursor_function =
				[this](ruis::mouse_cursor cursor) {
					auto& ww = get_impl(*this);
					ww.set_cursor(cursor);
				},
			.units = ruis::units(
				get_impl(window_pimpl).get_dots_per_inch(), //
				get_impl(window_pimpl).get_dots_per_pp()
			)
		}
	)),
	directory(get_application_directories(this->name))
{
#ifdef RUISAPP_RASPBERRYPI
	this->set_fullscreen(true);
#else
	this->update_window_rect(ruis::rect(0, 0, ruis::real(wp.dims.x()), ruis::real(wp.dims.y())));
#endif
}

namespace {

class xevent_waitable : public opros::waitable
{
public:
	xevent_waitable(Display* d) :
		opros::waitable(XConnectionNumber(d))
	{}
};

ruis::mouse_button button_number_to_enum(unsigned number)
{
	switch (number) {
		case 1: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::left;
		default:
		case 2: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::middle;
		case 3: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::right;
		case 4: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_up;
		case 5: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_down;
		case 6: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_left;
		case 7: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_right;
	}
}

const std::array<ruis::key, size_t(std::numeric_limits<uint8_t>::max()) + 1> key_code_map = {
	{//
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::escape, // 9
	 ruis::key::one, // 10
	 ruis::key::two, // 11
	 ruis::key::three, // 12
	 ruis::key::four, // 13
	 ruis::key::five, // 14
	 ruis::key::six, // 15
	 ruis::key::seven, // 16
	 ruis::key::eight, // 17
	 ruis::key::nine, // 18
	 ruis::key::zero, // 19
	 ruis::key::minus, // 20
	 ruis::key::equals, // 21
	 ruis::key::backspace, // 22
	 ruis::key::tabulator, // 23
	 ruis::key::q, // 24
	 ruis::key::w, // 25
	 ruis::key::e, // 26
	 ruis::key::r, // 27
	 ruis::key::t, // 28
	 ruis::key::y, // 29
	 ruis::key::u, // 30
	 ruis::key::i, // 31
	 ruis::key::o, // 32
	 ruis::key::p, // 33
	 ruis::key::left_square_bracket, // 34
	 ruis::key::right_square_bracket, // 35
	 ruis::key::enter, // 36
	 ruis::key::left_control, // 37
	 ruis::key::a, // 38
	 ruis::key::s, // 39
	 ruis::key::d, // 40
	 ruis::key::f, // 41
	 ruis::key::g, // 42
	 ruis::key::h, // 43
	 ruis::key::j, // 44
	 ruis::key::k, // 45
	 ruis::key::l, // 46
	 ruis::key::semicolon, // 47
	 ruis::key::apostrophe, // 48
	 ruis::key::grave, // 49
	 ruis::key::left_shift, // 50
	 ruis::key::backslash, // 51
	 ruis::key::z, // 52
	 ruis::key::x, // 53
	 ruis::key::c, // 54
	 ruis::key::v, // 55
	 ruis::key::b, // 56
	 ruis::key::n, // 57
	 ruis::key::m, // 58
	 ruis::key::comma, // 59
	 ruis::key::period, // 60
	 ruis::key::slash, // 61
	 ruis::key::right_shift, // 62
	 ruis::key::unknown,
	 ruis::key::left_alt, // 64
	 ruis::key::space, // 65
	 ruis::key::capslock, // 66
	 ruis::key::f1, // 67
	 ruis::key::f2, // 68
	 ruis::key::f3, // 69
	 ruis::key::f4, // 70
	 ruis::key::f5, // 71
	 ruis::key::f6, // 72
	 ruis::key::f7, // 73
	 ruis::key::f8, // 74
	 ruis::key::f9, // 75
	 ruis::key::f10, // 76
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::f11, // 95
	 ruis::key::f12, // 96
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::right_control, // 105
	 ruis::key::unknown,
	 ruis::key::print_screen, // 107
	 ruis::key::right_alt, // 108
	 ruis::key::unknown,
	 ruis::key::home, // 110
	 ruis::key::arrow_up, // 111
	 ruis::key::page_up, // 112
	 ruis::key::arrow_left, // 113
	 ruis::key::arrow_right, // 114
	 ruis::key::end, // 115
	 ruis::key::arrow_down, // 116
	 ruis::key::page_down, // 117
	 ruis::key::insert, // 118
	 ruis::key::deletion, // 119
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::pause, // 127
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::left_command, // 133
	 ruis::key::unknown,
	 ruis::key::menu, // 135
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown
	}
};

class key_event_unicode_provider : public ruis::gui::input_string_provider
{
	XIC& xic;
	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	XEvent& event;

public:
	key_event_unicode_provider(XIC& xic, XEvent& event) :
		xic(xic),
		event(event)
	{}

	std::u32string get() const override
	{
#ifndef X_HAVE_UTF8_STRING
#	error "no Xutf8stringlookup()"
#endif
		constexpr auto static_buf_size = 32;

		// the array is used to get data, so no need to initialize it
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
		std::array<char, static_buf_size> static_buf;

		std::vector<char> arr;
		auto buf = utki::make_span(static_buf);

		// the variable is initialized via output argument, so no need to initialize
		// it here NOLINTNEXTLINE(cppcoreguidelines-init-variables)
		Status status;

		int size = Xutf8LookupString(this->xic, &this->event.xkey, buf.begin(), int(buf.size() - 1), nullptr, &status);
		if (status == XBufferOverflow) {
			// allocate enough memory
			arr.resize(size + 1);
			buf = utki::make_span(arr);
			size = Xutf8LookupString(this->xic, &this->event.xkey, buf.begin(), int(buf.size() - 1), nullptr, &status);
		}
		ASSERT(size >= 0)
		ASSERT(buf.size() != 0)
		ASSERT(buf.size() > unsigned(size))

		//		TRACE(<< "KeyEventUnicodeResolver::Resolve(): size = " << size
		//<< std::endl)

		buf[size] = 0; // null-terminate

		switch (status) {
			case XLookupChars:
			case XLookupBoth:
				if (size == 0) {
					return {};
				}
				return utki::to_utf32(buf.data());
			default:
			case XBufferOverflow:
				ASSERT(false)
			case XLookupKeySym:
			case XLookupNone:
				break;
		}

		return {};
	}
};

} // namespace

void application::quit() noexcept
{
	auto& ww = get_impl(this->window_pimpl);

	ww.quit_flag.store(true);
}

int main(int argc, const char** argv)
{
	std::unique_ptr<ruisapp::application> app = create_app_unix(argc, argv);
	if (!app) {
		// Not an error. The app just did not show any GUI to the user.
		return 0;
	}

	ASSERT(app)

	auto& ww = get_impl(get_window_pimpl(*app));

	xevent_waitable xew(ww.display.display);

	opros::wait_set wait_set(2);

	wait_set.add(xew, {opros::ready::read}, &xew);
	wait_set.add(ww.ui_queue, {opros::ready::read}, &ww.ui_queue);

	// Sometimes the first Expose event does not come for some reason. It happens
	// constantly in some systems and never happens on all the others. So, render
	// everything for the first time.
	render(*app);

	while (!ww.quit_flag.load()) {
		// sequence:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle
		auto to_wait_ms = app->gui.update();
		render(*app);
		wait_set.wait(to_wait_ms);

		auto triggered_events = wait_set.get_triggered();

		bool ui_queue_ready_to_read = false;

		for (auto& ei : triggered_events) {
			if (ei.user_data == &ww.ui_queue) {
				ui_queue_ready_to_read = true;
			}
		}

		if (ui_queue_ready_to_read) {
			while (auto m = ww.ui_queue.pop_front()) {
				utki::log_debug([](auto& o) {
					o << "loop message" << std::endl;
				});
				m();
			}
		}

		ruis::vector2 new_win_dims(-1, -1);

		// NOTE: do not check 'read' flag for X event, for some reason when waiting
		// with 0 timeout it will never be set.
		//       Maybe some bug in XWindows, maybe something else.
		bool x_event_arrived = false;
		while (XPending(ww.display.display) > 0) {
			x_event_arrived = true;
			XEvent event;
			XNextEvent(ww.display.display, &event);
			// TRACE(<< "X event got, type = " << (event.type) << std::endl)
			switch (event.type) {
				case Expose:
					//						TRACE(<< "Expose X event
					// got" << std::endl)
					if (event.xexpose.count != 0) {
						break;
					}
					render(*app);
					break;
				case ConfigureNotify:
					// squash all window resize events into one, for that store the new
					// window dimensions and update the viewport later only once
					new_win_dims.x() = ruis::real(event.xconfigure.width);
					new_win_dims.y() = ruis::real(event.xconfigure.height);
					break;
				case KeyPress:
					//						TRACE(<< "KeyPress X
					// event got" << std::endl)
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						ruis::key key = key_code_map[std::uint8_t(event.xkey.keycode)];
						handle_key_event(*app, true, key);
						handle_character_input(*app, key_event_unicode_provider(ww.input_context, event), key);
					}
					break;
				case KeyRelease:
					//						TRACE(<< "KeyRelease X
					// event got" << std::endl)
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						ruis::key key = key_code_map[std::uint8_t(event.xkey.keycode)];

						// detect auto-repeated key events
						if (XEventsQueued(ww.display.display, QueuedAfterReading)) { // if there are other events queued
							XEvent nev;
							XPeekEvent(ww.display.display, &nev);

							if (nev.type == KeyPress && nev.xkey.time == event.xkey.time &&
								nev.xkey.keycode == event.xkey.keycode)
							{
								// key wasn't actually released
								handle_character_input(*app, key_event_unicode_provider(ww.input_context, nev), key);

								XNextEvent(ww.display.display,
										   &nev); // remove the key down event from queue
								break;
							}
						}

						handle_key_event(*app, false, key);
					}
					break;
				case ButtonPress:
					// utki::log_debug([&](auto&o){o << "ButtonPress X event got, button mask = " <<
					// event.xbutton.button << std::endl;}) utki::log_debug([&](auto&o){o <<
					// "ButtonPress X event got, x, y = " << event.xbutton.x << ", "
					// << event.xbutton.y << std::endl;})
					handle_mouse_button(
						*app,
						true,
						ruis::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case ButtonRelease:
					// utki::log_debug([&](auto&o){o << "ButtonRelease X event got, button mask = " <<
					// event.xbutton.button << std::endl;})
					handle_mouse_button(
						*app,
						false,
						ruis::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case MotionNotify:
					//						TRACE(<< "MotionNotify X
					// event got" << std::endl)
					handle_mouse_move(*app, ruis::vector2(event.xmotion.x, event.xmotion.y), 0);
					break;
				case EnterNotify:
					handle_mouse_hover(*app, true, 0);
					break;
				case LeaveNotify:
					handle_mouse_hover(*app, false, 0);
					break;
				case ClientMessage:
					//						TRACE(<< "ClientMessage
					// X event got" << std::endl)

					// probably a WM_DELETE_WINDOW event
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
						char* name = XGetAtomName(ww.display.display, event.xclient.message_type);
						if ("WM_PROTOCOLS"sv == name) {
							ww.quit_flag.store(true);
						}
						XFree(name);
					}
					break;
				default:
					// ignore
					break;
			}
		}

		// WORKAROUND: XEvent file descriptor becomes ready to read many times per
		// second, even if
		//             there are no events to handle returned by XPending(), so here
		//             we check if something meaningful actually happened and call
		//             render() only if it did
		if (triggered_events.size() != 0 && !x_event_arrived && !ui_queue_ready_to_read) {
			continue;
		}

		if (new_win_dims.is_positive_or_zero()) {
			update_window_rect(*app, ruis::rect(0, new_win_dims));
		}
	}

	wait_set.remove(ww.ui_queue);
	wait_set.remove(xew);

	return 0;
}

void application::set_fullscreen(bool enable)
{
#ifdef RUISAPP_RASPBERRYPI
	if (this->is_fullscreen()) {
		return;
	}
#endif
	if (enable == this->is_fullscreen()) {
		return;
	}

	auto& ww = get_impl(this->window_pimpl);

	Atom state_atom = XInternAtom(ww.display.display, "_NET_WM_STATE", False);
	Atom atom = XInternAtom(ww.display.display, "_NET_WM_STATE_FULLSCREEN", False);

	XEvent event;
	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.window = ww.window;
	event.xclient.message_type = state_atom;

	// data should be viewed as list of longs
	event.xclient.format = utki::byte_bits * 4;

	// use union from third-party code
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
	event.xclient.data.l[0] = enable ? 1 : 0;

	// use union from third-party code
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
	event.xclient.data.l[1] = long(atom);

	// use union from third-party code
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
	event.xclient.data.l[2] = 0;

	XSendEvent(
		ww.display.display,
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		DefaultRootWindow(ww.display.display),
		False,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&event
	);

	XFlush(ww.display.display);

	this->is_fullscreen_v = enable;
}

void application::set_mouse_cursor_visible(bool visible)
{
	get_impl(*this).set_cursor_visible(visible);
}

void application::swap_frame_buffers()
{
	auto& ww = get_impl(this->window_pimpl);

#ifdef RUISAPP_RENDER_OPENGL
	glXSwapBuffers(ww.display.display, ww.window);
#elif defined(RUISAPP_RENDER_OPENGLES)
	eglSwapBuffers(ww.egl_display, ww.egl_surface);
#else
#	error "Unknown graphics API"
#endif
}
