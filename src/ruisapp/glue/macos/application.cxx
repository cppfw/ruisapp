#include "application.hxx"

application_glue::application_glue(const utki::version_duplet& gl_version) :
	gl_version(gl_version),
	shared_gl_context_native_window(utki::make_shared<native_window>(
		this->gl_version,
		ruisapp::window_parameters{
			.dims = {1, 1},
			.fullscreen = false,
			.visible = false
},
		nullptr // no shared gl context
	)),
	resource_loader_ruis_rendering_context(
		utki::make_shared<ruis::render::opengl::context>(this->shared_gl_context_native_window)
	),
	common_shaders(this->resource_loader_ruis_rendering_context.get().make_shaders()),
	common_render_objects(
		utki::make_shared<ruis::render::renderer::objects>(this->resource_loader_ruis_rendering_context)
	),
	ruis_resource_loader( //
		utki::make_shared<ruis::resource_loader>(
			this->resource_loader_ruis_rendering_context, //
			this->common_render_objects
		)
	),
	ruis_style_provider( //
		utki::make_shared<ruis::style_provider>(this->ruis_resource_loader)
	)
{}

app_window::app_window(const ruisapp::window_parameters& window_params) :
	ruisapp::window(
		// TODO:
	)
{}
