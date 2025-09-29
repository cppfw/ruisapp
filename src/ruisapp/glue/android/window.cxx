#include "window.hxx"

native_window::native_window(
    const utki::version_duplet& gl_version,//
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

void native_window::swap_frame_buffers(){
    if(this->egl_surface.has_value()){
        this->egl_surface.value().swap_frame_buffers();
    }
}

void native_window::create_surface(){
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

    utki::assert(android_window, SL);

    // TODO: get android_window from somewhere
    utki::assert(android_window, SL);
    ANativeWindow_setBuffersGeometry(
        android_window, //
        0,
        0,
        format
    );
    
    this->egl_surface.emplace(
        this->egl_display,//
        this->egl_config,
        EGLNativeWindowType(android_window)
    );
}

void native_window::destroy_surface(){
    // according to
    // https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglMakeCurrent.xhtml
    // it is ok to destroy surface while EGL context is current, so here we do
    // not unbind the EGL context
    this->egl_surface.reset();
}

r4::vector2<unsigned> native_window::get_dims()
{
    if (!this->egl_surface.has_value()) {
        return {0, 0};
    }

    EGLint width, height;
    eglQuerySurface(this->egl_display.display,//
         this->egl_surface.value().surface, EGL_WIDTH, &width);
    eglQuerySurface(this->egl_display.display,//
         this->egl_surface.value().surface, EGL_HEIGHT, &height);
    return r4::vector2<unsigned>(width, height);
}
