#pragma once

#include <atomic>
#include <set>

#import <Cocoa/Cocoa.h>
#include <utki/destructable.hpp>

#include "../../application.hpp"

#include "display.hxx"
#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
public:
	const utki::shared_ref<native_window> ruis_native_window;

	app_window(
		utki::shared_ref<ruis::context> ruis_context, //
		utki::shared_ref<native_window> ruis_native_window
	);

	// needed for using with std::set
	bool operator<(const app_window& w) const noexcept
	{
		return this < &w;
	}

	// needed for using with std::set
	bool operator==(const app_window& w) const noexcept
	{
		return this == &w;
	}

	void resize(const ruis::vec2& dims);
};
} // namespace

namespace {
class application_glue : public utki::destructable
{
	const utki::version_duplet gl_version;

	const utki::shared_ref<native_window> shared_gl_context_native_window;
	const utki::shared_ref<ruis::render::context> resource_loader_ruis_rendering_context;
	const utki::shared_ref<const ruis::render::context::shaders> common_shaders;
	const utki::shared_ref<const ruis::render::renderer::objects> common_render_objects;
	const utki::shared_ref<ruis::resource_loader> ruis_resource_loader;
	const utki::shared_ref<ruis::style_provider> ruis_style_provider;

	std::set<utki::shared_ref<app_window>> windows;

public:
	std::atomic_bool quit_flag = false;

	const utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

	std::vector<utki::shared_ref<app_window>> windows_to_destroy;

	struct macos_application_wrapper {
		const NSApplication* application;

		macos_application_wrapper() :
			application([NSApplication sharedApplication])
		{
			if (!this->application) {
				throw std::runtime_error("failed to create NSApplication instance");
			}

			// TODO: why is this needed?
			[this->application activateIgnoringOtherApps:YES];
		}

		macos_application_wrapper(const macos_application_wrapper&) = delete;
		macos_application_wrapper& operator=(const macos_application_wrapper&) = delete;

		macos_application_wrapper(macos_application_wrapper&&) = delete;
		macos_application_wrapper& operator=(macos_application_wrapper&&) = delete;

		~macos_application_wrapper()
		{
			[this->application release];
		}
	} macos_application;

	application_glue(const utki::version_duplet& gl_version);

	ruisapp::window& make_window(ruisapp::window_parameters window_params);
	void destroy_window(app_window& w);

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
