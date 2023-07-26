/*
mordavokne - morda GUI adaptation layer

Copyright (C) 2016-2021  Ivan Gagis <igagis@gmail.com>

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
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <papki/fs_file.hpp>
#include <utki/string.hpp>
#include <utki/unicode.hpp>

#ifdef MORDAVOKNE_RENDER_OPENGL
#	include <GL/glew.h>
#	include <GL/glx.h>

#	include <morda/render/opengl/renderer.hpp>

#elif defined(MORDAVOKNE_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <GLES2/gl2.h>
#	ifdef MORDAVOKNE_RASPBERRYPI
#		include <bcm_host.h>
#	endif

#	include <morda/render/opengles/renderer.hpp>

#else
#	error "Unknown graphics API"
#endif

#include "../../application.hpp"
#include "../friend_accessors.cxx" // NOLINT(bugprone-suspicious-include)
#include "../unix_common.cxx" // NOLINT(bugprone-suspicious-include)
#include "../util.hxx"

using namespace mordavokne;

namespace {
const std::map<morda::mouse_cursor, unsigned> x_cursor_map = {
	{			   morda::mouse_cursor::arrow,            XC_left_ptr},
	{    morda::mouse_cursor::left_right_arrow,   XC_sb_h_double_arrow},
	{	   morda::mouse_cursor::up_down_arrow,   XC_sb_v_double_arrow},
	{morda::mouse_cursor::all_directions_arrow,               XC_fleur},
	{		   morda::mouse_cursor::left_side,           XC_left_side},
	{		  morda::mouse_cursor::right_side,          XC_right_side},
	{			morda::mouse_cursor::top_side,            XC_top_side},
	{		 morda::mouse_cursor::bottom_side,         XC_bottom_side},
	{	 morda::mouse_cursor::top_left_corner,     XC_top_left_corner},
	{    morda::mouse_cursor::top_right_corner,    XC_top_right_corner},
	{  morda::mouse_cursor::bottom_left_corner,  XC_bottom_left_corner},
	{ morda::mouse_cursor::bottom_right_corner, XC_bottom_right_corner},
	{		morda::mouse_cursor::index_finger,               XC_hand2},
	{				morda::mouse_cursor::grab,               XC_hand1},
	{			   morda::mouse_cursor::caret,               XC_xterm}
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
#ifdef MORDAVOKNE_RENDER_OPENGL
	GLXContext glContext;
#elif defined(MORDAVOKNE_RENDER_OPENGLES)
#	ifdef MORDAVOKNE_RASPBERRYPI
	EGL_DISPMANX_WINDOW_T rpiNativeWindow;
	DISPMANX_DISPLAY_HANDLE_T rpiDispmanDisplay;
	DISPMANX_UPDATE_HANDLE_T rpiDispmanUpdate;
	DISPMANX_ELEMENT_HANDLE_T rpiDispmanElement;
#	endif
	EGLDisplay eglDisplay;
	EGLSurface eglSurface;
	EGLContext eglContext;
#else
#	error "Unknown graphics API"
#endif
	struct cursor_wrapper {
		window_wrapper& owner;
		Cursor cursor;

		cursor_wrapper(window_wrapper& owner, morda::mouse_cursor c) :
			owner(owner)
		{
			if (c == morda::mouse_cursor::none) {
				std::array<char, 1> data = {0};

				Pixmap blank =
					XCreateBitmapFromData(this->owner.display.display, this->owner.window, data.data(), 1, 1);
				if (blank == None) {
					throw std::runtime_error(
						"application::XEmptyMouseCursor::XEmptyMouseCursor(): could not create bitmap"
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

	cursor_wrapper* cur_cursor = nullptr;
	bool cursor_visible = true;
	std::map<morda::mouse_cursor, std::unique_ptr<cursor_wrapper>> cursors;

	void apply_cursor(cursor_wrapper& c)
	{
		XDefineCursor(this->display.display, this->window, c.cursor);
	}

	cursor_wrapper* get_cursor(morda::mouse_cursor c)
	{
		auto i = this->cursors.find(c);
		if (i == this->cursors.end()) {
			i = this->cursors.insert(std::make_pair(c, std::make_unique<cursor_wrapper>(*this, c))).first;
		}
		return i->second.get();
	}

	void set_cursor(morda::mouse_cursor c)
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
			this->apply_cursor(*this->get_cursor(morda::mouse_cursor::none));
		}
	}

	XIM inputMethod;
	XIC input_context;

	nitki::queue ui_queue;

	volatile bool quitFlag = false;

	window_wrapper(const window_params& wp)
	{
#ifdef MORDAVOKNE_RENDER_OPENGL
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

			if (wp.buffers.get(window_params::buffer_type::depth)) {
				visual_attribs.push_back(GLX_DEPTH_SIZE);
				visual_attribs.push_back(utki::byte_bits * 3); // 24 bits per pixel for depth buffer
			}
			if (wp.buffers.get(window_params::buffer_type::stencil)) {
				visual_attribs.push_back(GLX_STENCIL_SIZE);
				visual_attribs.push_back(utki::byte_bits);
			}

			visual_attribs.push_back(None);

			int fbcount = 0;
			GLXFBConfig* fbc = glXChooseFBConfig(
				this->display.display,
				DefaultScreen(this->display.display),
				visual_attribs.data(),
				&fbcount
			);
			if (!fbc) {
				throw std::runtime_error("glXChooseFBConfig() returned empty list");
			}
			utki::scope_exit scope_exit_fbc([&fbc]() {
				XFree(fbc);
			});

			int best_fb_config_index = -1;
			int worst_fbc = -1;
			int best_num_samp = -1;
			// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
			int worst_num_samp = 999;

			for (int i = 0; i < fbcount; ++i) {
				XVisualInfo* vi = glXGetVisualFromFBConfig(
					this->display.display,
					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
					fbc[i]
				);
				if (vi) {
					// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
					int samp_buf, samples;
					glXGetFBConfigAttrib(
						this->display.display,
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
						fbc[i],
						GLX_SAMPLE_BUFFERS,
						&samp_buf
					);
					glXGetFBConfigAttrib(
						this->display.display,
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
						fbc[i],
						GLX_SAMPLES,
						&samples
					);

					if (best_fb_config_index < 0 || (samp_buf && samples > best_num_samp)) {
						best_fb_config_index = i;
						best_num_samp = samples;
					}
					if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp) {
						worst_fbc = i;
						worst_num_samp = samples;
					}
				}
				XFree(vi);
			}

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			best_fb_config = fbc[best_fb_config_index];
		}
#elif defined(MORDAVOKNE_RENDER_OPENGLES)
		// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
		this->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if (this->eglDisplay == EGL_NO_DISPLAY) {
			throw std::runtime_error("eglGetDisplay(): failed, no matching display connection found");
		}

		utki::scope_exit scope_exit_egl_display([this]() {
			eglTerminate(this->eglDisplay);
		});

		if (eglInitialize(this->eglDisplay, nullptr, nullptr) == EGL_FALSE) {
			eglTerminate(this->eglDisplay);
			throw std::runtime_error("eglInitialize() failed");
		}

		EGLConfig egl_config = nullptr;
		{
			// TODO: allow stencil and depth configuration etc. via window_params
			// Here specify the attributes of the desired configuration.
			// Below, we select an EGLConfig with at least 8 bits per color
			// component compatible with on-screen windows.
			const std::array<EGLint, 15> attribs = {
				EGL_SURFACE_TYPE,
				EGL_WINDOW_BIT,
				EGL_RENDERABLE_TYPE,
				EGL_OPENGL_ES2_BIT, // we want OpenGL ES 2.0
				EGL_BLUE_SIZE,
				8,
				EGL_GREEN_SIZE,
				8,
				EGL_RED_SIZE,
				8,
				EGL_ALPHA_SIZE,
				8,
				EGL_DEPTH_SIZE,
				16,
				EGL_NONE
			};

			// Here, the application chooses the configuration it desires. In this
			// sample, we have a very simplified selection process, where we pick
			// the first EGLConfig that matches our criteria.
			EGLint num_configs = 0;
			eglChooseConfig(this->eglDisplay, attribs.data(), &egl_config, 1, &num_configs);
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
#ifdef MORDAVOKNE_RENDER_OPENGL
		visual_info = glXGetVisualFromFBConfig(this->display.display, best_fb_config);
		if (!visual_info) {
			throw std::runtime_error("glXGetVisualFromFBConfig() failed");
		}
#elif defined(MORDAVOKNE_RENDER_OPENGLES)
#	ifdef MORDAVOKNE_RASPBERRYPI
		{
			int num_visuals;
			XVisualInfo vis_template;
			vis_template.screen = DefaultScreen(this->display.display); // LCD
			visual_info = XGetVisualInfo(this->display.display, VisualScreenMask, &vis_template, &num_visuals);
			if (!visual_info) {
				throw std::runtime_error("XGetVisualInfo() failed");
			}
		}
#	else
		{
			EGLint vid = 0;

			if (!eglGetConfigAttrib(this->eglDisplay, egl_config, EGL_NATIVE_VISUAL_ID, &vid)) {
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
			RootWindow(this->display.display, visual_info->screen),
			visual_info->visual,
			AllocNone
		);
		// TODO: check for error?
		utki::scope_exit scope_exit_color_map([this]() {
			XFreeColormap(this->display.display, this->color_map);
		});

		{
			XSetWindowAttributes attr;
			attr.colormap = this->color_map;
			attr.border_pixel = 0;
			attr.background_pixmap = None;
			attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
				| PointerMotionMask | ButtonMotionMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask;
			unsigned long fields = CWBorderPixel | CWColormap | CWEventMask;

			this->window = XCreateWindow(
				this->display.display,
				RootWindow(this->display.display, visual_info->screen),
				0,
				0,
				wp.dims.x(),
				wp.dims.y(),
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

		//====================
		// create GLX context

#ifdef MORDAVOKNE_RENDER_OPENGL
		// glXGetProcAddressARB() will retutn non-null pointer even if extension is not supported, so we
		// need to explicitly check for supported extensions.
		// SOURCE: https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

		auto glx_extensions_string =
			std::string_view(glXQueryExtensionsString(this->display.display, visual_info->screen));
		LOG([&](auto& o) {
			o << "glx_extensions_string = " << glx_extensions_string << std::endl;
		})

		auto glx_extensions = utki::split(glx_extensions_string);

		if (std::find(glx_extensions.begin(), glx_extensions.end(), "GLX_ARB_create_context") == glx_extensions.end()) {
			// GLX_ARB_create_context is not supported
			this->glContext = glXCreateContext(this->display.display, visual_info, nullptr, GL_TRUE);
		} else {
			// GLX_ARB_create_context is supported

			// NOTE: glXGetProcAddressARB() is guaranteed to be present in all GLX versions.
			//       glXGetProcAddress() is not guaranteed.
			// SOURCE: https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

			// NOLINTNEXTLINE(readability-identifier-naming)
			auto glXCreateContextAttribsARB = PFNGLXCREATECONTEXTATTRIBSARBPROC(glXGetProcAddressARB(
				static_cast<const GLubyte*>(static_cast<const void*>("glXCreateContextAttribsARB"))
			));

			if (!glXCreateContextAttribsARB) {
				// this should not happen since we checked extension presence, and anyway,
				// glXGetProcAddressARB() never returns nullptr according to
				// https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/
				// so, this check for null is just in case future version of GLX may return null
				throw std::runtime_error("glXCreateContextAttribsARB() not found");
			}

			auto ver = get_opengl_version_duplet(wp.graphics_api_request);

			static const std::array<int, 7> context_attribs = {
				GLX_CONTEXT_MAJOR_VERSION_ARB,
				ver.major,
				GLX_CONTEXT_MINOR_VERSION_ARB,
				ver.minor,
				GLX_CONTEXT_PROFILE_MASK_ARB,
				GLX_CONTEXT_CORE_PROFILE_BIT_ARB, // we don't need compatibility context
				None
			};

			this->glContext = glXCreateContextAttribsARB(
				this->display.display,
				best_fb_config,
				nullptr,
				GL_TRUE,
				context_attribs.data()
			);
		}

		// sync to ensure any errors generated are processed
		XSync(this->display.display, False);

		if (this->glContext == nullptr) {
			throw std::runtime_error("glXCreateContext() failed");
		}
		utki::scope_exit scope_exit_gl_context([this]() {
			glXMakeCurrent(this->display.display, None, nullptr);
			glXDestroyContext(this->display.display, this->glContext);
		});

		glXMakeCurrent(this->display.display, this->window, this->glContext);

		// disable v-sync via swap control extension

		if (std::find(glx_extensions.begin(), glx_extensions.end(), "GLX_EXT_swap_control") != glx_extensions.end()) {
			LOG([](auto& o) {
				o << "GLX_EXT_swap_control is supported\n";
			})

			// NOLINTNEXTLINE(readability-identifier-naming)
			auto glXSwapIntervalEXT = PFNGLXSWAPINTERVALEXTPROC(
				glXGetProcAddressARB(static_cast<const GLubyte*>(static_cast<const void*>("glXSwapIntervalEXT")))
			);

			ASSERT(glXSwapIntervalEXT)

			// disable v-sync
			glXSwapIntervalEXT(this->display.display, this->window, 0);
		} else if (std::find(glx_extensions.begin(), glx_extensions.end(), "GLX_MESA_swap_control") != glx_extensions.end())
		{
			LOG([](auto& o) {
				o << "GLX_MESA_swap_control is supported\n";
			})

			// NOLINTNEXTLINE(readability-identifier-naming)
			auto glXSwapIntervalMESA = PFNGLXSWAPINTERVALMESAPROC(
				glXGetProcAddressARB(static_cast<const GLubyte*>(static_cast<const void*>("glXSwapIntervalMESA")))
			);

			ASSERT(glXSwapIntervalMESA)

			// disable v-sync
			if (glXSwapIntervalMESA(0) != 0) {
				throw std::runtime_error("glXSwapIntervalMESA() failed");
			}
		} else {
			std::cout << "none of GLX_EXT_swap_control, GLX_MESA_swap_control GLX extensions are supported";
		}

		// sync to ensure any errors generated are processed
		XSync(this->display.display, False);

		//=============
		// init OpenGL

		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("GLEW initialization failed");
		}
#elif defined(MORDAVOKNE_RENDER_OPENGLES)

#	ifdef MORDAVOKNE_RASPBERRYPI
		{
			bcm_host_init();

			VC_RECT_T dst_rect, src_rect;

			uint32_t display_width, display_height;

			// create an EGL window surface, passing context width/height
			if (graphics_get_display_size(
					0, // LCD
					&display_width,
					&display_height
				)
				< 0)
			{
				throw std::runtime_error("graphics_get_display_size() failed");
			}

			dst_rect.x = 0;
			dst_rect.y = 0;
			dst_rect.width = display_width;
			dst_rect.height = display_height;

			src_rect.x = 0;
			src_rect.y = 0;
			src_rect.width = display_width << 16;
			src_rect.height = display_height << 16;

			this->rpiDispmanDisplay = vc_dispmanx_display_open(0); // 0 = LCD
			this->rpiDispmanUpdate = vc_dispmanx_update_start(0);

			this->rpiDispmanElement = vc_dispmanx_element_add(
				this->rpiDispmanUpdate,
				this->rpiDispmanDisplay,
				0, // layer
				&dst_rect,
				0, // src
				&src_rect,
				DISPMANX_PROTECTION_NONE,
				0, // alpha
				0, // clamp
				DISPMANX_NO_ROTATE // transform
			);

			this->rpiNativeWindow.element = this->rpiDispmanElement;
			this->rpiNativeWindow.width = display_width;
			this->rpiNativeWindow.height = display_height;
			vc_dispmanx_update_submit_sync(this->rpiDispmanUpdate);
		}
#	endif

		this->eglSurface = eglCreateWindowSurface(
			this->eglDisplay,
			egl_config,
#	ifdef MORDAVOKNE_RASPBERRYPI
			reinterpret_cast<EGLNativeWindowType>(&this->rpiNativeWindow),
#	else
			this->window,
#	endif
			nullptr
		);
		if (this->eglSurface == EGL_NO_SURFACE) {
			throw std::runtime_error("eglCreateWindowSurface() failed");
		}
		utki::scope_exit scope_exit_egl_surface([this]() {
			eglDestroySurface(this->eglDisplay, this->eglSurface);
		});

		{
			std::array<EGLint, 3> context_attrs = {
				EGL_CONTEXT_CLIENT_VERSION,
				2, // this is needed at least on Android, otherwise eglCreateContext() thinks that we want OpenGL
				   // ES 1.1, but we want 2.0
				EGL_NONE
			};

			this->eglContext = eglCreateContext(this->eglDisplay, egl_config, EGL_NO_CONTEXT, context_attrs.data());
			if (this->eglContext == EGL_NO_CONTEXT) {
				throw std::runtime_error("eglCreateContext() failed");
			}
		}

		if (eglMakeCurrent(this->eglDisplay, this->eglSurface, this->eglSurface, this->eglContext) == EGL_FALSE) {
			eglDestroyContext(this->eglDisplay, this->eglContext);
			throw std::runtime_error("eglMakeCurrent() failed");
		}
		utki::scope_exit scope_exit_egl_context([this]() {
			eglMakeCurrent(this->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroyContext(this->eglDisplay, this->eglContext);
		});

		// disable v-sync
		if (eglSwapInterval(this->eglDisplay, 0) != EGL_TRUE) {
			throw std::runtime_error("eglSwapInterval() failed");
		}
#else
#	error "Unknown graphics API"
#endif

		// initialize input method

		this->inputMethod = XOpenIM(this->display.display, nullptr, nullptr, nullptr);
		if (this->inputMethod == nullptr) {
			throw std::runtime_error("XOpenIM() failed");
		}
		utki::scope_exit scope_exit_input_method([this]() {
			XCloseIM(this->inputMethod);
		});

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		this->input_context = XCreateIC(
			this->inputMethod,
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

		scope_exit_input_context.reset();
		scope_exit_input_method.reset();
		scope_exit_window.reset();
		scope_exit_color_map.reset();
#ifdef MORDAVOKNE_RENDER_OPENGL
		scope_exit_gl_context.reset();
#elif defined(MORDAVOKNE_RENDER_OPENGLES)
		scope_exit_egl_display.reset();
		scope_exit_egl_surface.reset();
		scope_exit_egl_context.reset();
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

		XCloseIM(this->inputMethod);

#ifdef MORDAVOKNE_RENDER_OPENGL
		glXMakeCurrent(this->display.display, None, nullptr);
		glXDestroyContext(this->display.display, this->glContext);
#elif defined(MORDAVOKNE_RENDER_OPENGLES)
		eglMakeCurrent(this->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(this->eglDisplay, this->eglContext);
		eglDestroySurface(this->eglDisplay, this->eglSurface);
#else
#	error "Unknown graphics API"
#endif

		XDestroyWindow(this->display.display, this->window);
		XFreeColormap(this->display.display, this->color_map);

#ifdef MORDAVOKNE_RENDER_OPENGLES
		eglTerminate(this->eglDisplay);
#endif
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

namespace {
morda::real get_dots_per_inch(Display* display)
{
	int src_num = 0;

	constexpr auto mm_per_cm = 10;

	morda::real value = ((morda::real(DisplayWidth(display, src_num))
						  / (morda::real(DisplayWidthMM(display, src_num)) / morda::real(mm_per_cm)))
						 + (morda::real(DisplayHeight(display, src_num))
							/ (morda::real(DisplayHeightMM(display, src_num)) / morda::real(mm_per_cm))))
		/ 2;
	constexpr auto cm_per_inch = 2.54;
	value *= morda::real(cm_per_inch);
	return value;
}

morda::real get_dots_per_dp(Display* display)
{
	int src_num = 0;
	r4::vector2<unsigned> resolution(DisplayWidth(display, src_num), DisplayHeight(display, src_num));
	r4::vector2<unsigned> screen_size_mm(DisplayWidthMM(display, src_num), DisplayHeightMM(display, src_num));

	return application::get_pixels_per_dp(resolution, screen_size_mm);
}
} // namespace

application::application(std::string name, const window_params& wp) :
	name(std::move(name)),
	window_pimpl(std::make_unique<window_wrapper>(wp)),
	gui(utki::make_shared<morda::context>(
#ifdef MORDAVOKNE_RENDER_OPENGL
		utki::make_shared<morda::render_opengl::renderer>(),
#elif defined(MORDAVOKNE_RENDER_OPENGLES)
		utki::make_shared<morda::render_opengles::renderer>(),
#else
#	error "Unknown graphics API"
#endif
		utki::make_shared<morda::updater>(),
		[this](std::function<void()> a) {
			get_impl(get_window_pimpl(*this)).ui_queue.push_back(std::move(a));
		},
		[this](morda::mouse_cursor c) {
			auto& ww = get_impl(*this);
			ww.set_cursor(c);
		},
		get_dots_per_inch(get_impl(window_pimpl).display.display),
		::get_dots_per_dp(get_impl(window_pimpl).display.display)
	)),
	storage_dir(initialize_storage_dir(this->name))
{
#ifdef MORDAVOKNE_RASPBERRYPI
	this->set_fullscreen(true);
#else
	this->update_window_rect(morda::rectangle(0, 0, morda::real(wp.dims.x()), morda::real(wp.dims.y())));
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

morda::mouse_button button_number_to_enum(unsigned number)
{
	switch (number) {
		case 1: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return morda::mouse_button::left;
		default:
		case 2: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return morda::mouse_button::middle;
		case 3: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return morda::mouse_button::right;
		case 4: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return morda::mouse_button::wheel_up;
		case 5: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return morda::mouse_button::wheel_down;
		case 6: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return morda::mouse_button::wheel_left;
		case 7: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return morda::mouse_button::wheel_right;
	}
}

const std::array<morda::key, size_t(std::numeric_limits<uint8_t>::max()) + 1> key_code_map = {
	{morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::escape, // 9
 morda::key::one, // 10
 morda::key::two, // 11
 morda::key::three, // 12
 morda::key::four, // 13
 morda::key::five, // 14
 morda::key::six, // 15
 morda::key::seven, // 16
 morda::key::eight, // 17
 morda::key::nine, // 18
 morda::key::zero, // 19
 morda::key::minus, // 20
 morda::key::equals, // 21
 morda::key::backspace, // 22
 morda::key::tabulator, // 23
 morda::key::q, // 24
 morda::key::w, // 25
 morda::key::e, // 26
 morda::key::r, // 27
 morda::key::t, // 28
 morda::key::y, // 29
 morda::key::u, // 30
 morda::key::i, // 31
 morda::key::o, // 32
 morda::key::p, // 33
 morda::key::left_square_bracket, // 34
 morda::key::right_square_bracket, // 35
 morda::key::enter, // 36
 morda::key::left_control, // 37
 morda::key::a, // 38
 morda::key::s, // 39
 morda::key::d, // 40
 morda::key::f, // 41
 morda::key::g, // 42
 morda::key::h, // 43
 morda::key::j, // 44
 morda::key::k, // 45
 morda::key::l, // 46
 morda::key::semicolon, // 47
 morda::key::apostrophe, // 48
 morda::key::grave, // 49
 morda::key::left_shift, // 50
 morda::key::backslash, // 51
 morda::key::z, // 52
 morda::key::x, // 53
 morda::key::c, // 54
 morda::key::v, // 55
 morda::key::b, // 56
 morda::key::n, // 57
 morda::key::m, // 58
 morda::key::comma, // 59
 morda::key::period, // 60
 morda::key::slash, // 61
 morda::key::right_shift, // 62
 morda::key::unknown,
	 morda::key::left_alt, // 64
 morda::key::space, // 65
 morda::key::capslock, // 66
 morda::key::f1, // 67
 morda::key::f2, // 68
 morda::key::f3, // 69
 morda::key::f4, // 70
 morda::key::f5, // 71
 morda::key::f6, // 72
 morda::key::f7, // 73
 morda::key::f8, // 74
 morda::key::f9, // 75
 morda::key::f10, // 76
 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::f11, // 95
 morda::key::f12, // 96
 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::right_control, // 105
 morda::key::unknown,
	 morda::key::print_screen, // 107
 morda::key::right_alt, // 108
 morda::key::unknown,
	 morda::key::home, // 110
 morda::key::arrow_up, // 111
 morda::key::page_up, // 112
 morda::key::arrow_left, // 113
 morda::key::arrow_right, // 114
 morda::key::end, // 115
 morda::key::arrow_down, // 116
 morda::key::page_down, // 117
 morda::key::insert, // 118
 morda::key::deletion, // 119
 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::pause, // 127
 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::left_command, // 133
 morda::key::unknown,
	 morda::key::menu, // 135
 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown,
	 morda::key::unknown}
};

class key_event_unicode_provider : public morda::gui::input_string_provider
{
	XIC& xic;
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

		// the variable is initialized via output argument, so no need to initialize it here
		// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
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

		//		TRACE(<< "KeyEventUnicodeResolver::Resolve(): size = " << size << std::endl)

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

	ww.quitFlag = true;
}

int main(int argc, const char** argv)
{
	std::unique_ptr<mordavokne::application> app = create_app_unix(argc, argv);
	if (!app) {
		return 0;
	}

	ASSERT(app)

	auto& ww = get_impl(get_window_pimpl(*app));

	xevent_waitable xew(ww.display.display);

	opros::wait_set wait_set(2);

	wait_set.add(xew, {opros::ready::read}, &xew);
	wait_set.add(ww.ui_queue, {opros::ready::read}, &ww.ui_queue);

	// Sometimes the first Expose event does not come for some reason. It happens constantly in some systems and never
	// happens on all the others. So, render everything for the first time.
	render(*app);

	while (!ww.quitFlag) {
		wait_set.wait(app->gui.update());

		auto triggered_events = wait_set.get_triggered();

		bool ui_queue_ready_to_read = false;

		for (auto& ei : triggered_events) {
			if (ei.user_data == &ww.ui_queue) {
				ui_queue_ready_to_read = true;
			}
		}

		if (ui_queue_ready_to_read) {
			while (auto m = ww.ui_queue.pop_front()) {
				LOG([](auto& o) {
					o << "loop message" << std::endl;
				})
				m();
			}
		}

		morda::vector2 new_win_dims(-1, -1);

		// NOTE: do not check 'read' flag for X event, for some reason when waiting with 0 timeout it will never be set.
		//       Maybe some bug in XWindows, maybe something else.
		bool x_event_arrived = false;
		while (XPending(ww.display.display) > 0) {
			x_event_arrived = true;
			XEvent event;
			XNextEvent(ww.display.display, &event);
			// TRACE(<< "X event got, type = " << (event.type) << std::endl)
			switch (event.type) {
				case Expose:
					//						TRACE(<< "Expose X event got" << std::endl)
					if (event.xexpose.count != 0) {
						break;
					}
					render(*app);
					break;
				case ConfigureNotify:
					//						TRACE(<< "ConfigureNotify X event got" << std::endl)
					// squash all window resize events into one, for that store the new window dimensions and update the
					// viewport later only once
					new_win_dims.x() = morda::real(event.xconfigure.width);
					new_win_dims.y() = morda::real(event.xconfigure.height);
					break;
				case KeyPress:
					//						TRACE(<< "KeyPress X event got" << std::endl)
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						morda::key key = key_code_map[std::uint8_t(event.xkey.keycode)];
						handle_key_event(*app, true, key);
						handle_character_input(*app, key_event_unicode_provider(ww.input_context, event), key);
					}
					break;
				case KeyRelease:
					//						TRACE(<< "KeyRelease X event got" << std::endl)
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						morda::key key = key_code_map[std::uint8_t(event.xkey.keycode)];

						// detect auto-repeated key events
						if (XEventsQueued(ww.display.display, QueuedAfterReading)) { // if there are other events queued
							XEvent nev;
							XPeekEvent(ww.display.display, &nev);

							if (nev.type == KeyPress && nev.xkey.time == event.xkey.time
								&& nev.xkey.keycode == event.xkey.keycode)
							{
								// key wasn't actually released
								handle_character_input(*app, key_event_unicode_provider(ww.input_context, nev), key);

								XNextEvent(ww.display.display, &nev); // remove the key down event from queue
								break;
							}
						}

						handle_key_event(*app, false, key);
					}
					break;
				case ButtonPress:
					// LOG([&](auto&o){o << "ButtonPress X event got, button mask = " << event.xbutton.button <<
					// std::endl;}) LOG([&](auto&o){o << "ButtonPress X event got, x, y = " << event.xbutton.x << ", "
					// << event.xbutton.y << std::endl;})
					handle_mouse_button(
						*app,
						true,
						morda::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case ButtonRelease:
					// LOG([&](auto&o){o << "ButtonRelease X event got, button mask = " << event.xbutton.button <<
					// std::endl;})
					handle_mouse_button(
						*app,
						false,
						morda::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case MotionNotify:
					//						TRACE(<< "MotionNotify X event got" << std::endl)
					handle_mouse_move(*app, morda::vector2(event.xmotion.x, event.xmotion.y), 0);
					break;
				case EnterNotify:
					handle_mouse_hover(*app, true, 0);
					break;
				case LeaveNotify:
					handle_mouse_hover(*app, false, 0);
					break;
				case ClientMessage:
					//						TRACE(<< "ClientMessage X event got" << std::endl)
					// probably a WM_DELETE_WINDOW event
					{
						char* name = XGetAtomName(ww.display.display, event.xclient.message_type);
						if (*name == *"WM_PROTOCOLS") {
							ww.quitFlag = true;
						}
						XFree(name);
					}
					break;
				default:
					// ignore
					break;
			}
		}

		// WORKAROUND: XEvent file descriptor becomes ready to read many times per second, even if
		//             there are no events to handle returned by XPending(), so here we check if something
		//             meaningful actually happened and call render() only if it did
		if (triggered_events.size() != 0 && !x_event_arrived && !ui_queue_ready_to_read) {
			continue;
		}

		if (new_win_dims.is_positive_or_zero()) {
			update_window_rect(*app, morda::rectangle(0, new_win_dims));
		}

		render(*app);
	}

	wait_set.remove(ww.ui_queue);
	wait_set.remove(xew);

	return 0;
}

void application::set_fullscreen(bool enable)
{
#ifdef MORDAVOKNE_RASPBERRYPI
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

#ifdef MORDAVOKNE_RENDER_OPENGL
	glXSwapBuffers(ww.display.display, ww.window);
#elif defined(MORDAVOKNE_RENDER_OPENGLES)
	eglSwapBuffers(ww.eglDisplay, ww.eglSurface);
#else
#	error "Unknown graphics API"
#endif
}
