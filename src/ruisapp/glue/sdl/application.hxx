/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2025  Ivan Gagis <igagis@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* ================ LICENSE END ================ */

#pragma once

#include <atomic>

#include <utki/destructable.hpp>

#include "../../application.hpp"

#include "display.hxx"
#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
public:
	utki::shared_ref<native_window> ruis_native_window;

	app_window(
		utki::shared_ref<ruis::context> ruis_context, //
		utki::shared_ref<native_window> ruis_native_window
	) :
		ruisapp::window(std::move(ruis_context)),
		ruis_native_window(std::move(ruis_native_window))
	{
		utki::assert(
			[&]() {
				ruis::render::native_window& w1 = this->ruis_native_window.get();
				ruis::render::native_window& w2 = this->gui.context.get().window();
				return &w1 == &w2;
			},
			SL
		);
	}

	ruis::vector2 new_win_dims{-1, -1};
};
} // namespace

namespace {
class application_glue : public utki::destructable
{
public:
	const utki::shared_ref<display_wrapper> display = utki::make_shared<display_wrapper>();

private:
	const utki::version_duplet gl_version;

// only one window allowed on emscripten, so we don't create hidden window for shared GL context
#if CFG_OS_NAME != CFG_OS_NAME_EMSCRIPTEN
	const utki::shared_ref<native_window> shared_gl_context_native_window;
	const utki::shared_ref<ruis::render::context> resource_loader_ruis_rendering_context;
	const utki::shared_ref<const ruis::render::context::shaders> common_shaders;
	const utki::shared_ref<const ruis::render::renderer::objects> common_render_objects;
	const utki::shared_ref<ruis::resource_loader> ruis_resource_loader;
	const utki::shared_ref<ruis::style_provider> ruis_style_provider;
#endif

	std::map<
		native_window::window_id_type, //
		utki::shared_ref<app_window> //
		>
		windows;

public:
	std::vector<utki::shared_ref<app_window>> windows_to_destroy;

	utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

	std::atomic_bool quit_flag = false;

	application_glue(const utki::version_duplet& gl_version);

	ruisapp::window& make_window(ruisapp::window_parameters window_params);
	void destroy_window(native_window::window_id_type id);

	app_window* get_window(native_window::window_id_type id);

	size_t get_num_windows() const noexcept
	{
		return this->windows.size();
	}

	// render all windows if needed
	void render();

	void apply_new_win_dims();
};
} // namespace

namespace {
inline application_glue& get_glue(ruisapp::application& app)
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast, "false-positive")
	return static_cast<application_glue&>(app.pimpl.get());
}
} // namespace

namespace {
inline application_glue& get_glue()
{
	return get_glue(ruisapp::application::inst());
}
} // namespace
