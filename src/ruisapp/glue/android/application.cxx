#include "application.hxx"

#include <ruis/render/opengles/context.hpp>

#include "asset_file.hxx"
#include "globals.hxx"
#include "window.hxx"

application_glue::application_glue(utki::version_duplet gl_version) :
	gl_version(std::move(gl_version))
{}

app_window& application_glue::make_window(ruisapp::window_parameters window_params)
{
	if (this->window.has_value()) {
		throw std::logic_error(
			"application::make_window(): one window already exists, only one window allowed on android"
		);
	}

	utki::assert(!this->window.has_value(), SL);

	auto ruis_native_window = utki::make_shared<native_window>(
		this->gl_version, //
		window_params
	);

	auto rendering_context = utki::make_shared<ruis::render::opengles::context>(ruis_native_window);

	auto common_render_objects = utki::make_shared<ruis::render::renderer::objects>(rendering_context);
	auto common_shaders = rendering_context.get().make_shaders();

	auto ruis_resource_loader = utki::make_shared<ruis::resource_loader>(
		rendering_context, //
		common_render_objects
	);

	auto ruis_style_provider = utki::make_shared<ruis::style_provider>(std::move(ruis_resource_loader));

	auto ruis_context = utki::make_shared<ruis::context>(ruis::context::parameters{
		.post_to_ui_thread_function =
			[this](std::function<void()> procedure) {
				// TODO:
				// if (PostMessage(
				// 		NULL, // post message to UI thread's message queue
				// 		WM_USER,
				// 		0, // no wParam
				// 		// NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-pro-type-reinterpret-cast)
				// 		reinterpret_cast<LPARAM>(new std::remove_reference_t<decltype(procedure)>(std::move(procedure)))
				// 	) == 0)
				// {
				// 	throw std::runtime_error("PostMessage(): failed");
				// }
			},
		.updater = this->updater,
		.renderer = utki::make_shared<ruis::render::renderer>(
			std::move(rendering_context),
			std::move(common_shaders),
			std::move(common_render_objects)
		),
		.style_provider = std::move(ruis_style_provider),
		// TODO:
		// .units = ruis::units(
		// 	ruis_native_window.get().get_dots_per_inch(), //
		// 	ruis_native_window.get().get_dots_per_pp()
		// )
	});

	this->window.emplace(
		std::move(ruis_context), //
		std::move(ruis_native_window)
	);

	// TODO: window surface will likely be created later, so defer setting viewport till that time?
	// ruisapp_window.get().gui.set_viewport( //
	// 	ruis::rect(
	// 		0, //
	// 		0,
	// 		ruis::real(window_params.dims.x()),
	// 		ruis::real(window_params.dims.y())
	// 	)
	// );

	utki::assert(this->window.has_value(), SL);
	return this->window.value();
}

void application_glue::render()
{
	if (this->window.has_value()) {
		this->window.value().render();
	}
}

namespace {
ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	auto& glob = get_glob();

	auto storage_dir = papki::as_dir(glob.java_functions.get_storage_dir());

	ruisapp::application::directories dirs;

	dirs.cache = utki::cat(storage_dir, "cache/");
	dirs.config = utki::cat(storage_dir, "config/");
	dirs.state = utki::cat(storage_dir, "state/");

	return dirs;
}
} // namespace

ruisapp::application::application(parameters params) :
	application(
		utki::make_unique<application_glue>(params.graphics_api_version), //
		get_application_directories(params.name),
		std::move(params)
	)
{}

std::unique_ptr<papki::file> ruisapp::application::get_res_file(std::string_view path) const
{
	utki::assert(globals_wrapper::native_activity, SL);
	utki::assert(globals_wrapper::native_activity->assetManager, SL);

	return std::make_unique<asset_file>(
		globals_wrapper::native_activity->assetManager, //
		path
	);
}

ruisapp::window& ruisapp::application::make_window(ruisapp::window_parameters window_params)
{
	auto& glue = get_glue(*this);
	return glue.make_window(std::move(window_params));
}

void ruisapp::application::destroy_window(ruisapp::window& w)
{
	auto& glue = get_glue(*this);

	utki::assert(dynamic_cast<app_window*>(&w), SL);
	glue.destroy_window();
}

void ruisapp::application::quit() noexcept
{
	utki::assert(globals_wrapper::native_activity, SL);
	ANativeActivity_finish(globals_wrapper::native_activity);
}

void ruisapp::application::show_virtual_keyboard() noexcept
{
	// NOTE:
	// ANativeActivity_showSoftInput(native_activity,
	// ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED); did not work for some reason.

	auto& glob = get_glob();

	glob.java_functions.show_virtual_keyboard();
}

void ruisapp::application::hide_virtual_keyboard() noexcept
{
	// NOTE:
	// ANativeActivity_hideSoftInput(native_activity,
	// ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS); did not work for some reason

	auto& glob = get_glob();

	glob.java_functions.hide_virtual_keyboard();
}
