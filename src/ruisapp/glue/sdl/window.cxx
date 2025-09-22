#include "window.hxx"

namespace {
ruis::real get_display_dpi(int display_index = 0)
{
	float dpi = ruis::units::default_dots_per_inch;
	if (SDL_GetDisplayDPI(
			display_index, //
			&dpi, // diagonal dpi
			nullptr, // no horizontal dpi
			nullptr // no vertical dpi
		) != 0)
	{
		throw std::runtime_error(utki::cat("Could not get SDL display DPI, SDL Error: ", SDL_GetError()));
	}
	// std::cout << "get_dpi(): dpi = " << dpi << std::endl;
	return ruis::real(dpi);
}
} // namespace

namespace {
ruis::real get_display_scaling_factor(int display_index = 0)
{
	using std::round;
	return round(get_display_dpi(display_index) / ruis::units::default_dots_per_inch);
}
} // namespace

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
namespace {
static bool on_emscripten_canvas_size_changed_callback(int event_type, const void* reserved, void* user_data)
{
	if (!user_data) {
		std::cout << "emscripten_get_canvas_element_size(#canvas): user_data is nullptr" << std::endl;
		return false;
	}
	auto* ww = reinterpret_cast<sdl_window_wrapper*>(user_data);

	double width = 0;
	double height = 0;

	if (auto res = emscripten_get_element_css_size("#canvas", &width, &height); res != EMSCRIPTEN_RESULT_SUCCESS) {
		std::cout << "emscripten_get_canvas_element_size(#canvas): failed, error = " << res << std::endl;
		return false;
	}

	// std::cout << "emscripten_get_canvas_element_size(#canvas): new canvas size = " << width << " " << height << std::endl;

	SDL_SetWindowSize(
		ww->window, //
		int(width),
		int(height)
	);

	return true;
}
} // namespace
#endif

native_window::sdl_window_wrapper::sdl_window_wrapper(
	const utki::version_duplet& gl_version, //
	const ruisapp::window_parameters& window_params,
	bool is_shared_context_window
) :
	dpi(get_display_dpi()),
	scale_factor(get_display_scaling_factor()),
	window([&]() {
#ifdef RUISAPP_RENDER_OPENGL
		SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK, //
			SDL_GL_CONTEXT_PROFILE_CORE
		);
#elif defined(RUISAPP_RENDER_OPENGLES)
		SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK, //
			SDL_GL_CONTEXT_PROFILE_ES
		);
#else
#	error "Unknown graphics API"
#endif
		{
			auto ver = gl_version;
			if (ver.major < 2) {
				// default OpenGL version is 2.0
				// TODO: set default version for non-OpenGL APIs
				ver.major = 2;
				ver.minor = 0;
			}
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, ver.major);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, ver.minor);
		}

#ifdef DEBUG
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
		// Lock orientation if requested
		if (window_params.orientation != ruisapp::orientation::dynamic) {
			int emsc_orient = [&]() {
				switch (window_params.orientation) {
					default:
					case ruisapp::orientation::landscape:
						return EMSCRIPTEN_ORIENTATION_LANDSCAPE_PRIMARY;
					case ruisapp::orientation::portrait:
						return EMSCRIPTEN_ORIENTATION_PORTRAIT_PRIMARY;
				}
			}();
			emscripten_lock_orientation(emsc_orient);
		}

		// Change to soft fullscreen mode before creating the window to set correct OpenGL viewport initially.
		{
			EmscriptenFullscreenStrategy strategy{};
			strategy = {
				.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
				.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
				.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
				.canvasResizedCallback = &on_emscripten_canvas_size_changed_callback,
				.canvasResizedCallbackUserData = this
			};
			emscripten_enter_soft_fullscreen(
				"#canvas", //
				&strategy
			);
		}

		auto dims = []() {
			double width;
			double height;

			if (auto res = emscripten_get_element_css_size(
					"#canvas", //
					&width,
					&height
				);
				res != EMSCRIPTEN_RESULT_SUCCESS)
			{
				throw std::runtime_error(utki::cat(
					"emscripten_get_canvas_element_size(#canvas): failed, error = ", //
					res
				));
			}
			return ruis::vec2(
				ruis::real(width), //
				ruis::real(height)
			);
		}();
#else
		auto dims = window_params.dims.to<ruis::real>();
		dims *= this->scale_factor;
#endif

		// std::cout << "dims = " << dims << std::endl;

		if (is_shared_context_window) {
			SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
		} else {
			SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
		}

		SDL_Window* window = SDL_CreateWindow(
			window_params.title.c_str(), //
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			int(dims.x()),
			int(dims.y()),
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
		);
		if (!window) {
			std::runtime_error(utki::cat(
				"Could not create SDL window, SDL_Error: ", //
				SDL_GetError()
			));
		}

		return window;
	}())
{
	if (!is_shared_context_window) {
		SDL_HideWindow(this->window);
	}
}

native_window::sdl_window_wrapper::~sdl_window_wrapper()
{
	SDL_DestroyWindow(this->window);
}

native_window::sdl_gl_context_wrapper::sdl_gl_context_wrapper(sdl_window_wrapper& sdl_window) :
	context([&]() {
		SDL_GLContext c = SDL_GL_CreateContext(sdl_window.window);
		if (!c) {
			throw std::runtime_error(utki::cat("Could not create OpenGL context, SDL Error: ", SDL_GetError()));
		}
		return c;
	}())
{}

native_window::sdl_gl_context_wrapper::~sdl_gl_context_wrapper()
{
	SDL_GL_DeleteContext(this->context);
}

native_window::native_window(
	utki::shared_ref<display_wrapper> display,
	const utki::version_duplet& gl_version, //
	const ruisapp::window_parameters& window_params,
	native_window* shared_gl_context_native_window
) :
	display(std::move(display)),
	sdl_window(
		gl_version, //
		window_params,
		// If we are creating a shared context then
		// before crating the SDL window we need to set SDL_GL_SHARE_WITH_CURRENT_CONTEXT to 1.
		shared_gl_context_native_window != nullptr
	),
	sdl_gl_context(
		shared_gl_context_native_window ?
										// In SDL, to make shared context we need to:
			// - set SDL_GL_SHARE_WITH_CURRENT_CONTEXT to 1 when creating shared context window
			// - supply the window with which to share the context when creating the shared context
			// After the context is created it can be bound to any other window.
			shared_gl_context_native_window->sdl_window
										: this->sdl_window
	)
{
	if (SDL_GL_GetCurrentContext() == NULL) {
		if (SDL_GL_MakeCurrent(
				this->sdl_window.window, //
				this->sdl_gl_context.context
			) != 0)
		{
			throw std::runtime_error(utki::cat(
				"SDL_GL_MakeCurrent() failed: ", //
				SDL_GetError()
			));
		}
	}

#ifdef RUISAPP_RENDER_OPENGL
	if (shared_gl_context_native_window == nullptr) {
		// this is the first GL context created, initialize GLEW
		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("Could not initialize GLEW");
		}
	}
#endif
}

ruis::vec2 native_window::get_dims() const noexcept
{
	int width = 0;
	int height = 0;
	SDL_GetWindowSize(
		this->sdl_window.window, //
		&width,
		&height
	);

	auto dims = ruis::vec2(ruis::real(width), ruis::real(height));

	// std::cout << "actual window size = " << dims << std::endl;

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
	dims *= ww.window.scale_factor;
#endif

	return dims;
}

void native_window::set_mouse_cursor(ruis::mouse_cursor c)
{
	this->display.get().set_cursor(c);
}
