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
	const display_wrapper& display,
	const device_context_wrapper& device_context,
	const ruisapp::window_parameters& window_params,
	const utki::version_duplet& gl_version,
	HGLRC shared_context
) :
	context([&]() {
		std::array<int, 15> pixel_format_attributes = {
			// Can draw to a window
			WGL_DRAW_TO_WINDOW_ARB,
			GL_TRUE,
			// Supports OpenGL rendering
			WGL_SUPPORT_OPENGL_ARB,
			GL_TRUE,
			// Use double buffering
			WGL_DOUBLE_BUFFER_ARB,
			GL_TRUE,
			// Use RGBA pixel type
			WGL_PIXEL_TYPE_ARB,
			WGL_TYPE_RGBA_ARB,
			// 32 bit color depth
			WGL_COLOR_BITS_ARB,
			32,
			// depth buffer (use 16 bit depth)
			WGL_DEPTH_BITS_ARB,
			window_params.buffers.get(ruisapp::buffer::depth) ? BYTE(utki::byte_bits * 2)
															  : BYTE(0),
			// stencil buffer (use 8 bit depth)
			WGL_STENCIL_BITS_ARB,
			window_params.buffers.get(ruisapp::buffer::stencil) ? BYTE(utki::byte_bits) : BYTE(0),
			// terminating zero for the list
			0
		};

		int format_index = 0;
		UINT num_formats= 0;

		if (!display.wgl_procedures.wgl_choose_pixel_format_arb(
			device_context.context, //
			pixel_format_attributes.data(), // int attributes
			NULL, // no float attributes
			1, // max num formats to return
			&format_index, // return index of the chosen format
			&num_formats // number of matching formats
		)	) {
			throw std::runtime_error("wglChoosePixelFormatARB() failed");
		}

		if (num_formats == 0) {
			throw std::runtime_error("no suitable pixel formats found");
		}

		 PIXELFORMATDESCRIPTOR pfd;
		 if (DescribePixelFormat(device_context.context, //
			 format_index, sizeof(pfd), &pfd) == 0) {
			 throw std::runtime_error("DescribePixelFormat() failed");
		}

		if (!SetPixelFormat(device_context.context,//
			format_index, &pfd)) {
			throw std::runtime_error("SetPixelFormat() failed");
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

		std::array<int, 7> rendering_context_attribs = {
			WGL_CONTEXT_MAJOR_VERSION_ARB,
			graphics_api_version.major,
			WGL_CONTEXT_MINOR_VERSION_ARB,
			graphics_api_version.minor,
			WGL_CONTEXT_PROFILE_MASK_ARB,
			WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			//WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0 // terminate the list
		};

		HGLRC hrc = display.wgl_procedures.wgl_create_context_attribs_arb(device_context.context,//
			shared_context,
			rendering_context_attribs.data());

		//static PIXELFORMATDESCRIPTOR pfd = {
		//	sizeof(PIXELFORMATDESCRIPTOR),
		//	1, // Version number of the structure, should be 1
		//	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		//	BYTE(PFD_TYPE_RGBA),
		//	BYTE(utki::byte_bits * 4), // 32 bit color depth
		//	BYTE(0),
		//	BYTE(0),
		//	BYTE(0),
		//	BYTE(0),
		//	BYTE(0),
		//	BYTE(0), // color bits ignored
		//	BYTE(0), // no alpha buffer
		//	BYTE(0), // shift bit ignored
		//	BYTE(0), // no accumulation buffer
		//	BYTE(0),
		//	BYTE(0),
		//	BYTE(0),
		//	BYTE(0), // accumulation bits ignored
		//	window_params.buffers.get(ruisapp::buffer::depth) ? BYTE(utki::byte_bits * 2)
		//													  : BYTE(0), // 16 bit depth buffer
		//	window_params.buffers.get(ruisapp::buffer::stencil) ? BYTE(utki::byte_bits) : BYTE(0),
		//	BYTE(0), // no auxiliary buffer
		//	BYTE(PFD_MAIN_PLANE), // main drawing layer
		//	BYTE(0), // reserved
		//	0,
		//	0,
		//	0 // layer masks ignored
		//};

		//int pixel_format = ChoosePixelFormat(
		//	device_context.context, //
		//	&pfd
		//);
		//if (!pixel_format) {
		//	throw std::runtime_error("Could not find suitable pixel format");
		//}

		//if (!SetPixelFormat(
		//		device_context.context, //
		//		pixel_format,
		//		&pfd
		//	))
		//{
		//	throw std::runtime_error("Could not set pixel format");
		//}

		//auto hrc = wglCreateContext(device_context.context);

		if (hrc == NULL) {
			DWORD error_code = GetLastError();

			std::string error_message;
			switch (error_code) {
				case ERROR_INVALID_VERSION_ARB:
					error_message = "ERROR_INVALID_VERSION_ARB";
					break;
				case ERROR_INVALID_PROFILE_ARB:
					error_message = "ERROR_INVALID_PROFILE_ARB";
					break;
				case ERROR_INVALID_OPERATION:
					error_message = "ERROR_INVALID_OPERATION ";
					break;
				case ERROR_DC_NOT_FOUND:
					error_message = "ERROR_DC_NOT_FOUND";
					break;
				case ERROR_INVALID_PIXEL_FORMAT:
					error_message = "ERROR_INVALID_PIXEL_FORMAT";
					break;
				case ERROR_NO_SYSTEM_RESOURCES:
					error_message = "ERROR_NO_SYSTEM_RESOURCES";
					break;
				case ERROR_INVALID_PARAMETER:
					error_message = "ERROR_INVALID_PARAMETER";
					break;
				case ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB:
					error_message = "ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB";
					break;
				default:
					error_message = "unknown error";
					break;
			}

			auto message = utki::cat("Failed to create OpenGL rendering context, error = ", //
					error_code, ": ", error_message );
			throw std::runtime_error(
				std::move(message));



			//LPVOID msg_buf;

			//const char* error_message = "no info";

			//if (FormatMessage(
			//	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			//	FORMAT_MESSAGE_FROM_SYSTEM |
			//	FORMAT_MESSAGE_IGNORE_INSERTS,
			//	NULL,
			//	error_code,
			//	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			//	(LPTSTR) &msg_buf, // TODO: static_cast?
			//	0, 
			//	NULL) == 0) {
			//	error_message="message format failed";
			//}
			//else {
			//	error_message = (LPCTSTR)msg_buf;// TODO: static_cast?
			//}

			//utki::scope_exit msg_buf_scope_exit([&]() {
			//	LocalFree(msg_buf);
			//});

			//auto message = utki::cat("Failed to create OpenGL rendering context, error = ", //
			//		error_code, ": ", error_message );
			//throw std::runtime_error(
			//	std::move(message));
		}

		//if(shared_context != NULL){
		//	wglShareLists(hrc, //
		//		shared_context);
		//}

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
		this->display,
		this->device_context, //
		window_params,
		gl_version,
		shared_gl_context_native_window ? shared_gl_context_native_window->opengl_context.context : NULL
	)
#elif defined(RUISAPP_RENDER_OPENGLES)
	egl_display(EGLNativeDisplayType(this->device_context.context)),
	egl_config(
		this->egl_display, //
		gl_version,
		window_params
	),
	egl_surface(this->egl_display,//
		this->egl_config, EGLNativeWindowType(this->window.handle)),
	egl_context(
		this->egl_display,//
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
