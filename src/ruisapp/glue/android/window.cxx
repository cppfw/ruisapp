#include "window.hxx"

native_window::native_window(
	const utki::version_duplet& gl_version, //
	const ruisapp::window_parameters& window_params
) :
	egl_config(
		this->egl_display, //
		gl_version,
		window_params
	),
	egl_context(
		this->egl_display, //
		gl_version,
		this->egl_config,
		EGL_NO_CONTEXT // no shared context
	)
{}

void native_window::bind_rendering_context()
{
	if (this->egl_surface.has_value()) {
		eglMakeCurrent(
			this->egl_display.display, //
			this->egl_surface.value().surface,
			this->egl_surface.value().surface,
			this->egl_context.context
		);
	} else {
		if (this->egl_display.extensions.get(egl::extension::khr_surfaceless_context)) {
			eglMakeCurrent(
				this->egl_display.display, //
				EGL_NO_SURFACE,
				EGL_NO_SURFACE,
				this->egl_context.context
			);
		} else {
			// KHR_surfaceless_context EGL extension is not available, create a dummy pbuffer surface to make the context current
			if (!this->egl_dummy_surface.has_value()) {
				this->egl_dummy_surface.emplace(
					this->egl_display, //
					egl_config
				);
			}
			eglMakeCurrent(
				this->egl_display.display, //
				this->egl_dummy_surface.value().surface,
				this->egl_dummy_surface.value().surface,
				this->egl_context.context
			);
		}
	}
}

void native_window::swap_frame_buffers()
{
	if (!this->egl_surface.has_value()) {
		return;
	}

	this->egl_surface.value().swap_frame_buffers();
}

void native_window::create_surface(ANativeWindow& android_window)
{
	utki::assert(!this->egl_surface.has_value(), SL);

	EGLint format;

	// EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	// guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	// As soon as we picked a EGLConfig, we can safely reconfigure the
	// ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
	if (eglGetConfigAttrib(
			this->egl_display.display, //
			this->egl_config.config,
			EGL_NATIVE_VISUAL_ID,
			&format
		) == EGL_FALSE)
	{
		throw std::runtime_error("eglGetConfigAttrib() failed");
	}

	// if both buffer width and height are 0 then it will be sized to the window dimensions
	ANativeWindow_setBuffersGeometry(
		&android_window, //
		0, // buffer width in pixels
		0, // buffer height in pixels
		format
	);

	this->egl_surface.emplace(
		this->egl_display, //
		this->egl_config,
		EGLNativeWindowType(&android_window)
	);

	if (eglGetCurrentContext() == this->egl_context.context) {
		// This context is current, that means it was bound to a EGL_NO_SURFACE or to a dummy surface.
		// Need to re-bind it to the just created surface.
		this->bind_rendering_context();
	}
}

void native_window::destroy_surface()
{
	bool context_was_bound = eglGetCurrentContext() == this->egl_context.context;

	// according to
	// https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglMakeCurrent.xhtml
	// it is ok to destroy surface while EGL context is current, so here we do
	// not unbind the EGL context
	this->egl_surface.reset();

	if (context_was_bound) {
		// Need to re-bind the context to EGL_NO_SURFACE or to a dummy surface, as by ruisapp convention,
		// if there are any rendering contexts exist then there should always be one of them bound.
		this->bind_rendering_context();
	}
}

r4::vector2<unsigned> native_window::get_dims()
{
	if (!this->egl_surface.has_value()) {
		return {0, 0};
	}

	return this->egl_surface.value().get_dims();
}
