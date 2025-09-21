#pragma once

#include <atomic>
#include <map>

#include "../../application.hpp"

#include "display.hxx"
#include "window.hxx"

namespace {
class app_window : public ruisapp::window{
public:
	const utki::shared_ref<native_window> ruis_native_window;

	app_window(
		utki::shared_ref<ruis::context> ruis_context, //
		utki::shared_ref<native_window> ruis_native_window
	):
		ruisapp::window(std::move(ruis_context)),
		ruis_native_window(std::move(ruis_native_window))
	{}
};
}

namespace {
class application_glue : public utki::destructable
{
	const utki::version_duplet gl_version;

	utki::shared_ref<display_wrapper> display = utki::make_shared<display_wrapper>();

	const utki::shared_ref<native_window> shared_gl_context_native_window;
	const utki::shared_ref<ruis::render::context> resource_loader_ruis_rendering_context;
	const utki::shared_ref<const ruis::render::context::shaders> common_shaders;
	const utki::shared_ref<const ruis::render::renderer::objects> common_render_objects;
	const utki::shared_ref<ruis::resource_loader> ruis_resource_loader;
	const utki::shared_ref<ruis::style_provider> ruis_style_provider;

	std::map<
		native_window::window_id_type,
		utki::shared_ref<app_window>> windows;
public:
	std::atomic_bool quit_flag = false;

	const utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

	std::vector<utki::shared_ref<app_window>> windows_to_destroy;

	application_glue(const utki::version_duplet& gl_version);

	ruisapp::window& make_window(ruisapp::window_parameters window_params);
	void destroy_window(native_window::window_id_type id);

	app_window* get_window(native_window::window_id_type id);

	void render();
};
} // namespace

namespace {
inline application_glue& get_glue(ruisapp::application& app)
{
	return static_cast<application_glue&>(app.pimpl.get());
}
} // namespace

namespace {
inline application_glue& get_glue()
{
	return get_glue(ruisapp::application::inst());
}
} // namespace
