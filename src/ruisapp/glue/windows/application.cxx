#include "application.hxx"

#include <Shlobj.h> // needed for SHGetFolderPathA()

namespace {
ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	// the variable is initialized via output argument, so no need
	// to initialize it here
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	std::array<CHAR, MAX_PATH> path;
	if (SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, path.data()) != S_OK) {
		throw std::runtime_error("failed to get user's profile directory.");
	}

	path.back() = '\0'; // null-terminate the string just in case

	std::string home_dir(path.data(), strlen(path.data()));
	ASSERT(!home_dir.empty())

	std::replace(
		home_dir.begin(), //
		home_dir.end(),
		'\\',
		'/'
	);

	home_dir = papki::as_dir(home_dir);

	home_dir.append(1, '.').append(app_name).append(1, '/');

	ruisapp::application::directories dirs;

	dirs.cache = utki::cat(home_dir, "cache/");
	dirs.config = utki::cat(home_dir, "config/");
	dirs.state = utki::cat(home_dir, "state/");

	return dirs;
}
} // namespace

application_glue::application_glue(const utki::version_duplet& gl_version) :
	gl_version(gl_version),
	shared_gl_context_native_window(utki::make_shared<native_window>(
		this->display,
		this->gl_version,
		ruisapp::window_parameters{
			.dims = {1, 1},
			.fullscreen = false,
			.visible = false
},
		nullptr // no shared gl context
	)),
	resource_loader_ruis_rendering_context([&]() {
		utki::logcat_debug("application_glue::application_glue(): creating shared gl context", '\n');
#ifdef RUISAPP_RENDER_OPENGL
		using context_type = ruis::render::opengl::context;
#elif defined(RUISAPP_RENDER_OPENGLES)
		using context_type = ruis::render::opengles::context;
#else
#	error "Unknown graphics API"
#endif
		auto c = utki::make_shared<context_type>(this->shared_gl_context_native_window);
		utki::logcat_debug("application_glue::application_glue(): shared gl context created", '\n');
		return c;
	}()),
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

void application_glue::render() {
	for (auto& w : this->windows) {
		w.second.get().render();
	}
}

app_window* application_glue::get_window(native_window::window_id_type id) {
	auto i = this->windows.find(id);
	if (i == this->windows.end()) {
		return nullptr;
	}

	return &i->second.get();
}

ruisapp::window& application_glue::make_window(ruisapp::window_parameters window_params) {
	auto ruis_native_window = utki::make_shared<native_window>(
		this->display,
		this->gl_version,
		window_params,
		&this->shared_gl_context_native_window.get()
	);

	auto ruis_context = utki::make_shared<ruis::context>(ruis::context::parameters{
		.post_to_ui_thread_function =
			[this](std::function<void()> procedure) {
				if (PostMessage(
 						NULL, // post message to UI thread's message queue
 						WM_USER,
 						0, // no wParam
 						// NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-pro-type-reinterpret-cast)
 						reinterpret_cast<LPARAM>(
 							new std::remove_reference_t<decltype(procedure)>(std::move(procedure))
 						)
 					) == 0)
 				{
 					throw std::runtime_error("PostMessage(): failed");
 				}
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
		.style_provider = this->ruis_style_provider,
		.units = ruis::units(
 				ruis_native_window.get().get_dots_per_inch(), //
 				ruis_native_window.get().get_dots_per_pp()
 			)
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

void application_glue::destroy_window(native_window::window_id_type id){
	auto i = this->windows.find(id);
	utki::assert(i != this->windows.end(), SL);

	this->windows_to_destroy.push_back(std::move(i->second));
	this->windows.erase(i);
}

ruisapp::application::application(parameters params) :
	application(
		utki::make_unique<application_glue>(params.graphics_api_version), //
		get_application_directories(params.name),
		std::move(params)
	)
{}

void ruisapp::application::quit() noexcept{
	auto& glue = get_glue(*this);

	PostQuitMessage(
		0 // exit code
	);
}

ruisapp::window& ruisapp::application::make_window(ruisapp::window_parameters window_params) {
	auto& glue = get_glue(*this);
	return glue.make_window(std::move(window_params));
}

void ruisapp::application::destroy_window(ruisapp::window& w) {
	auto& glue = get_glue(*this);

	utki::assert(dynamic_cast<app_window*>(&w), SL);
	auto& app_win = static_cast<app_window&>(w);
	glue.destroy_window(app_win.ruis_native_window.get().get_id());
}
