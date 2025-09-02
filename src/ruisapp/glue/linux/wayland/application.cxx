#include "application.hxx"

#include <ruis/widget/widget.hpp>

#ifdef RUISAPP_RENDER_OPENGL
#	include <ruis/render/opengl/context.hpp>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <ruis/render/opengles/context.hpp>

#else
#	error "Unknown graphics API"
#endif

void app_window::resize(const r4::vector2<uint32_t>& dims)
{
	this->cur_window_dims = dims;

	utki::log_debug([&](auto& o) {
		o << "resize window to " << std::dec << dims << std::endl;
	});

	this->ruis_native_window.get().resize(dims);

	// TODO: update ruis::context::units
	// auto& app = ruisapp::inst();
	// auto& units = app.gui.context.get().units;
	// units.set_dots_per_pp(this->scale);
	// units.set_dots_per_inch(ruis::real(sd.dpi));

	this->gui.set_viewport(ruis::rect(0, dims.to<ruis::real>()));
}

void app_window::notify_outputs_changed()
{
	if (this->outputs_changed_message_pending) {
		return;
	}

	this->outputs_changed_message_pending = true;

	auto& glue = get_glue();

	glue.ui_queue.push_back([this]() {
		this->outputs_changed_message_pending = false;

		// this call will update ruis::context::units values
		this->resize(this->cur_window_dims);

		// reload widgets hierarchy due to possible update of ruis::context::units values
		this->gui.get_root().reload();
	});
}

ruisapp::application::application(parameters params) :
	application(
		utki::make_unique<application_glue>(params.graphics_api_version), //
		get_application_directories(params.name),
		std::move(params)
	)
{}

void ruisapp::application::quit() noexcept
{
	auto& glue = get_glue(*this);
	glue.quit_flag.store(true);
}

ruisapp::window& ruisapp::application::make_window(const window_parameters& window_params)
{
	auto& glue = get_glue(*this);
	return glue.make_window(window_params);
}

void ruisapp::application::destroy_window(ruisapp::window& w)
{
	auto& glue = get_glue(*this);

	utki::assert(dynamic_cast<app_window*>(&w), SL);
	glue.destroy_window(static_cast<app_window&>(w));
}

ruisapp::window& application_glue::make_window(const ruisapp::window_parameters& window_params)
{
	auto ruis_native_window = utki::make_shared<native_window>(
		this->display, //
		this->gl_version,
		window_params,
		&this->shared_gl_context_native_window.get()
	);

	auto ruis_context = utki::make_shared<ruis::context>(ruis::context::parameters{
		.post_to_ui_thread_function =
			[this](std::function<void()> proc) {
				this->ui_queue.push_back(std::move(proc));
			},
		.updater = this->updater,
		.renderer = utki::make_shared<ruis::render::renderer>(
#ifdef RUISAPP_RENDER_OPENGL
			utki::make_shared<ruis::render::opengl::context>(ruis_native_window),
#elif defined(RUISAPP_RENDER_OPENGLES)
			utki::make_shared<ruis::render::opengles::context>(ruis_native_window),
#else
#	error "Unknown graphics API"
#endif
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

	auto res = this->windows.insert( //
		std::make_pair(
			ruisapp_window.get().ruis_native_window.get().get_id(), //
			std::move(ruisapp_window)
		)
	);
	utki::assert(res.second, SL);

	return res.first->second.get();
}

void application_glue::destroy_window(app_window& w)
{
	auto i = this->windows.find(w.ruis_native_window.get().get_id());
	utki::assert(i != this->windows.end(), SL);
	this->windows.erase(i);
	// TODO: defer window destruction by pushing destroy proc to ui queue?
}
