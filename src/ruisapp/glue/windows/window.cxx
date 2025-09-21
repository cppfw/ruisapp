#include "window.hxx"

#include <cstdint>
#include <stdexcept>

#include <utki/windows.hpp>

native_window::window_wrapper::window_wrapper(
	const display_wrapper::window_class_wrapper& window_class,
	const ruisapp::window_parameters& window_params
	) :
	handle([&]() {
		auto hwnd = CreateWindowEx(
			WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, // extended style
			window_class.window_class_name,
			window_params.title.c_str(),
			WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, // x
			0, // y
			// width
			int(window_params.dims.x()) + 2 * GetSystemMetrics(SM_CXSIZEFRAME),
			// height
			int(window_params.dims.y()) + GetSystemMetrics(SM_CYCAPTION) + 2 * GetSystemMetrics(SM_CYSIZEFRAME),
			NULL, // no parent window
			NULL, // no menu
			GetModuleHandle(NULL),
			nullptr // do not pass anything to WM_CREATE
		);

		if (hwnd == NULL) {
			// TODO: add error information to the exception message using GetLastError() and FormatMessage()
			throw std::runtime_error("Failed to create a window");
		}

		return hwnd;
	}())
{
	if(window_params.visible){
		ShowWindow(this->handle,//
			SW_SHOW);
	}
}

native_window::window_wrapper::~window_wrapper()
{
	auto res = DestroyWindow(this->handle);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Failed to destroy window";
		},
		SL
	);
}

native_window::device_context_wrapper::device_context_wrapper(const window_wrapper& window) :
	window(window),
	context([&]() {
		auto hdc = GetDC(this->window.handle);
		if (hdc == NULL) {
			throw std::runtime_error("Failed to create a OpenGL device context");
		}
		return hdc;
	}())
{}

native_window::device_context_wrapper::~device_context_wrapper()
{
	auto res = ReleaseDC(
		this->window.handle, //
		this->context
	);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Failed to release device context";
		},
		SL
	);
}

#ifdef RUISAPP_RENDER_OPENGL
native_window::opengl_context_wrapper::opengl_context_wrapper(
	const device_context_wrapper& device_context,
	const ruisapp::window_parameters& window_params,
	HGLRC shared_context
) :
	context([&]() {
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
			window_params.buffers.get(ruisapp::buffer::depth) ? BYTE(utki::byte_bits * 2)
															  : BYTE(0), // 16 bit depth buffer
			window_params.buffers.get(ruisapp::buffer::stencil) ? BYTE(utki::byte_bits) : BYTE(0),
			BYTE(0), // no auxiliary buffer
			BYTE(PFD_MAIN_PLANE), // main drawing layer
			BYTE(0), // reserved
			0,
			0,
			0 // layer masks ignored
		};

		int pixel_format = ChoosePixelFormat(
			device_context.context, //
			&pfd
		);
		if (!pixel_format) {
			throw std::runtime_error("Could not find suitable pixel format");
		}

		if (!SetPixelFormat(
				device_context.context, //
				pixel_format,
				&pfd
			))
		{
			throw std::runtime_error("Could not set pixel format");
		}

		// TODO: use wglCreateContextAttribsARB() which allows specifying opengl version
		auto hrc = wglCreateContext(device_context.context);
		if (hrc == NULL) {
			// TODO: add error information to the exception message using GetLastError() and FormatMessage()
			throw std::runtime_error("Failed to create OpenGL rendering context");
		}

		if(shared_context != NULL){
			wglShareLists(hrc, //
				shared_context);
		}

		return hrc;
	}())
{}

native_window::opengl_context_wrapper::~opengl_context_wrapper()
{
	if (wglGetCurrentContext() == this->context) {
		// unbind context
		auto res = wglMakeCurrent(NULL, NULL);
		utki::assert(
			res,
			[&](auto& o) {
				o << "Deactivating OpenGL rendering context failed";
			},
			SL
		);
	}

	auto res = wglDeleteContext(this->context);
	utki::assert(
		res,
		[&](auto& o) {
			o << "Releasing OpenGL rendering context failed";
		},
		SL
	);
}
#endif

native_window::native_window(
	utki::shared_ref<display_wrapper> display,
	const utki::version_duplet& gl_version,
	const ruisapp::window_parameters& window_params,
	native_window* shared_gl_context_native_window
) :
	display(std::move(display)),
	window(
		shared_gl_context_native_window ? this->display.get().regular_window_class : this->display.get().dummy_window_class, //
		window_params
	),
	device_context(this->window),
#ifdef RUISAPP_RENDER_OPENGL
	opengl_context(
		this->device_context, //
		window_params,
		shared_gl_context_native_window ? shared_gl_context_native_window->opengl_context.context : NULL
	)
#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_display(EGLNativeDisplayType(this->device_context.context)),
	egl_config(
		this->egl_display, //
		gl_version,
		window_params
	),
	egl_surface(this->egl_display, this->egl_config, EGLNativeWindowType(this->window.handle)),
	egl_context(
		this->egl_display,
		gl_version,
		this->egl_config,
		shared_gl_context_native_window ? shared_gl_context_native_window->egl_context.context : EGL_NO_CONTEXT
	)
#endif
{
#ifdef RUISAPP_RENDER_OPENGL
	if (wglGetCurrentContext() == NULL) {
		auto res = wglMakeCurrent(
			this->device_context.context, //
			this->opengl_context.context
		);

		if (!res) {
			throw std::runtime_error("Failed to activate OpenGL rendering context");
		}

		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("GLEW initialization failed");
		}
	}
#endif
}

void native_window::bind_rendering_context() {
#ifdef RUISAPP_RENDER_OPENGL
	auto res = wglMakeCurrent(
		this->device_context.context, //
		this->opengl_context.context
	);
	if (!res) {
		throw std::runtime_error("Failed to activate OpenGL rendering context");
	}
#elif defined(RUISAPP_RENDER_OPENGLES)
	if (eglMakeCurrent(
			this->egl_display.display,
			this->egl_surface.surface,
			this->egl_surface.surface,
			this->egl_context.context
		) == EGL_FALSE)
	{
		throw std::runtime_error("eglMakeCurrent() failed");
	}
#else
#	error "Unknown graphics API"
#endif
}

void native_window::swap_frame_buffers()
{
#ifdef RUISAPP_RENDER_OPENGL
	SwapBuffers(this->device_context.context);
#elif defined(RUISAPP_RENDER_OPENGLES)
	eglSwapBuffers(
		this->egl_display.display, //
		this->egl_surface.surface
	);
#else
#	error "Unknown graphics API"
#endif
}
