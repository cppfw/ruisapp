#include "application.hxx"

app_window::app_window(
	utki::shared_ref<ruis::context> ruis_context, //
	utki::shared_ref<native_window> ruis_native_window
) :
	ruisapp::window(std::move(ruis_context)),
	ruis_native_window(std::move(ruis_native_window))
{
	this->ruis_native_window.get().set_app_window(this);
}

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

ruisapp::window& application_glue::make_window(const ruisapp::window_parameters& window_params)
{
	auto ruis_native_window =
		utki::make_shared<native_window>(this->gl_version, window_params, &this->shared_gl_context_native_window.get());

	auto ruis_context = utki::make_shared<ruis::context>(ruis::context::parameters{
		.post_to_ui_thread_function =
			[this](std::function<void()> proc) {
				this->ui_queue.push_back(std::move(proc));
			},
		.updater = this->updater,
		.renderer = utki::make_shared<ruis::render::renderer>(
			utki::make_shared<ruis::render::opengl::context>(ruis_native_window), //
			this->common_shaders,
			this->common_render_objects
		),
		.style_provider = this->ruis_style_provider
	});

	auto ruisapp_window = utki::make_shared<app_window>(
		std::move(ruis_context), //
		std::move(ruis_native_window)
	);

	ruisapp_window.get().gui.set_viewport( //
		ruis::rect(
			0, //
			0,
			ruis::real(window_params.dims.x()),
			ruis::real(window_params.dims.y())
		)
	);

	auto res = this->windows.insert(std::move(ruisapp_window));
	utki::assert(res.second, SL);

	return res.first->get();
}

void application_glue::destroy_window(app_window& w)
{
	auto i = this->windows.find(w);
	utki::assert(i != this->windows.end(), SL);

	// Defer actual window object destruction until next main loop cycle,
	// for that put the window to the list of windows to destroy.
	this->windows_to_destroy.push_back(std::move(*i));

	this->windows.erase(i);
}
