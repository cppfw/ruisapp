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
#include <ruis/widget/widget.hpp>
#include <utki/views.hpp>

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

ruis::sides<ruis::styled<ruis::length>> app_window::get_insets()
{
	// Each styled value references the shared style_value owned by this window, so
	// updates via set_insets() are observed without re-querying.
	ruis::sides<ruis::styled<ruis::length>> ret;
	for (auto [style_value, styled_value] : utki::views::zip(this->insets, ret)) {
		styled_value = ruis::styled<ruis::length>(style_value);
	}
	return ret;
}

void app_window::set_insets(ruis::sides<ruis::real> insets)
{
	bool changed = false;

	for (auto [inset, inset_value] : utki::views::zip(this->insets, insets)) {
		auto& cur = inset.get().get_value();
		auto new_value = ruis::length::make_px(inset_value);
		if (cur == new_value) {
			continue;
		}
		cur = new_value;
		changed = true;
	}

	if (!changed) {
		return;
	}

	// The inset values are referenced by the application's widgets (e.g. as borders of a ruis::padding),
	// but those widgets have no idea that the values they reference have just changed, so nothing would
	// re-layout them. Invalidate the whole widget subtree, so that on the next render the layout is
	// re-done from the root and every widget picks up the new inset values.
	// (Just reloading the root is not enough: invalidate_layout() only marks the root and its ancestors,
	// and the layout algorithms skip clean children when laying out a parent, so the change would not
	// reach the widgets nested inside the inset padding.)
	// NOTE: the root widget is always present (it is a dummy widget until the application sets its own
	// one), so it is safe to call this at any time.
	this->gui.get_root().invalidate_subtree_layout();
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

	bool fullscreen = window_params.fullscreen;

	auto ruis_native_window = utki::make_shared<native_window>(
		this->gl_version, //
		std::move(window_params)
	);

	// apply the initial fullscreen state requested by the application.
	// on android this enters/exits immersive mode: hides/shows the system
	// status bar and the navigation bar.
	ruis_native_window.get().set_fullscreen(fullscreen);

	auto rendering_context = utki::make_shared<ruis::render::opengles::context>(ruis_native_window);

	auto common_render_objects = utki::make_shared<ruis::render::renderer::objects>(rendering_context);
	auto common_shaders = rendering_context.get().make_shaders();

	auto ruis_resource_loader = utki::make_shared<ruis::resource_loader>(
		rendering_context, //
		common_render_objects
	);

	// custom style provider is not supported on android
	utki::assert(window_params.style_provider == nullptr, SL);

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
			[]() -> float {
				auto& glob = get_glob();
				float pixels_per_dp = glob.java_functions.get_pixels_per_dp();
				utki::logcat_debug("pixels_per_dp = ", pixels_per_dp, '\n');
				return pixels_per_dp;
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

	auto storage_dir = fsif::as_dir(glob.java_functions.get_storage_dir());

	ruisapp::application::directories dirs;

	dirs.cache = utki::cat(storage_dir, "cache/");
	dirs.data = utki::cat(storage_dir, "data/");
	dirs.config = utki::cat(storage_dir, "config/");
	dirs.state = utki::cat(storage_dir, "state/");

	return dirs;
}
} // namespace

ruisapp::application::application(parameters params) :
	application(
		{utki::make_unique<application_glue>(params.graphics_api_version), //
		 get_application_directories(params.name),
		 std::move(params)}
	)
{}

utki::unique_ref<fsif::file> ruisapp::application::get_res_file(std::string_view path) const
{
	utki::assert(globals_wrapper::native_activity, SL);
	utki::assert(globals_wrapper::native_activity->assetManager, SL);

	return utki::make_unique<asset_file>(
		globals_wrapper::native_activity->assetManager, //
		path
	);
}

ruisapp::window& ruisapp::application::make_window_internal(ruisapp::window_parameters window_params)
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

std::vector<std::reference_wrapper<ruisapp::window>> ruisapp::application::get_windows()
{
	auto& glue = get_glue(*this);

	std::vector<std::reference_wrapper<ruisapp::window>> ret;
	if (auto* w = glue.get_window()) {
		ret.emplace_back(*w);
	}

	return ret;
}

const std::shared_ptr<ruis::style_provider>& ruisapp::application::get_shared_style_provider()
{
	// Android supports creating only one window and there is no shared gl context.
	// So there is no style provider shared between windows.
	static const std::shared_ptr<ruis::style_provider> null_sp;
	return null_sp;
}

void ruisapp::application::quit() noexcept
{
	utki::assert(globals_wrapper::native_activity, SL);
	ANativeActivity_finish(globals_wrapper::native_activity);
}
