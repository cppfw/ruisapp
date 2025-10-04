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

#include "application.hxx"

#include <ruis/render/opengles/context.hpp>

#include "asset_file.hxx"
#include "globals.hxx"
#include "window.hxx"

void app_window::set_win_rect(const ruis::rect& r)
{
	this->cur_win_rect = r;

	auto& glob = get_glob();

	ruis::rect viewport_rect = r;
	viewport_rect.p.y() = glob.cur_window_dims.y() - viewport_rect.y2();
	this->gui.set_viewport(viewport_rect);
}

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
				auto& glob = get_glob();
				glob.ui_queue.push_back(std::move(procedure));
			},
		.updater = this->updater,
		.renderer = utki::make_shared<ruis::render::renderer>(
			std::move(rendering_context),
			std::move(common_shaders),
			std::move(common_render_objects)
		),
		.style_provider = std::move(ruis_style_provider),
		.units = ruis::units(
			[]() -> float {
				auto& glob = get_glob();
				float dpi = glob.java_functions.get_dots_per_inch();
				utki::logcat_debug("dpi = ", dpi, '\n');
				return dpi;
			}(),
			[ruis_native_window]() -> float {
				auto& glob = get_glob();

				auto dims_px = glob.java_functions.get_screen_resolution();
				auto dims_mm =
					(dims_px.to<float>() / glob.java_functions.get_dots_per_inch()) * float(utki::mm_per_inch);
				return ruisapp::application::get_pixels_per_pp(
					dims_px, // dimensions in pixels
					dims_mm.to<unsigned>() // dimensions in millimeters
				);
			}()
		)
	});

	this->window.emplace(
		std::move(ruis_context), //
		std::move(ruis_native_window)
	);

	// NOTE: Window surface will be created later,
	// when android creates an android window for the activity.
	// Android will also send content_rect_changed notifications to the
	// activity, and there we will set the GL viewport, so no need to do it here.

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
	utki::assert(
		false,
		[](auto& o) {
			o << "ruisapp::application::destroy_window(): programmatically destroying window on android is not allowed. The window is destroyed along with the activity.";
		},
		SL
	);
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
