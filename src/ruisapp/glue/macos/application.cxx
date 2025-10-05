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

namespace {
ruis::real get_dots_per_inch()
{
	NSScreen* screen = [NSScreen mainScreen];
	NSDictionary* description = [screen deviceDescription];
	NSSize display_pixel_size = [[description objectForKey:NSDeviceSize] sizeValue];
	CGSize display_physical_size = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);

	ruis::real value = ruis::real(
		((display_pixel_size.width * 10.0f / display_physical_size.width) +
		 (display_pixel_size.height * 10.0f / display_physical_size.height)) /
		2.0f
	);
	value *= 2.54f;
	return value;
}
} // namespace

namespace {
ruis::real get_dots_per_pp()
{
	NSScreen* screen = [NSScreen mainScreen];
	NSDictionary* description = [screen deviceDescription];
	NSSize display_pixel_size = [[description objectForKey:NSDeviceSize] sizeValue];
	CGSize display_physical_size = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);

	r4::vector2<unsigned> resolution(display_pixel_size.width, display_pixel_size.height);
	r4::vector2<unsigned> screen_size_mm(display_physical_size.width, display_physical_size.height);

	return application::get_pixels_per_pp(resolution, screen_size_mm);
}
} // namespace

app_window::app_window(
	utki::shared_ref<ruis::context> ruis_context, //
	utki::shared_ref<native_window> ruis_native_window
) :
	ruisapp::window(std::move(ruis_context)),
	ruis_native_window(std::move(ruis_native_window))
{
	this->ruis_native_window.get().set_app_window(this);
}

void app_window::resize(const ruis::vec2& dims)
{
	auto& natwin = this->ruis_native_window.get();

	natwin.resize(dims);

	// TODO: take scale into account?
	// auto& units = this->gui.context.get().units;
	// units.set_dots_per_pp(natwin.get_scale());
	// units.set_dots_per_inch(natwin.get_dpi());

	this->gui.set_viewport( //
		ruis::rect(
			0, //
			dims
			// TODO: take scale into account?
			// (dims * natwin.get_scale()).to<ruis::real>()
		)
	);
}

application_glue::application_glue(const utki::version_duplet& gl_version) :
	gl_version(gl_version),
	shared_gl_context_native_window(utki::make_shared<native_window>(
		this->gl_version,
		ruisapp::window_parameters{
			.dims = {1, 1},
			.fullscreen = false
},
		nullptr // no shared gl context
	)),
	resource_loader_ruis_rendering_context([&]() {
		utki::logcat_debug("application_glue::application_glue(): creating shared gl context", '\n');
		auto c = utki::make_shared<ruis::render::opengl::context>(this->shared_gl_context_native_window);
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
{
	utki::logcat_debug("application_glue::application_glue(): application_glue created", '\n');
}

ruisapp::window& application_glue::make_window(ruisapp::window_parameters window_params)
{
	auto ruis_native_window = utki::make_shared<native_window>(
		this->gl_version, //
		window_params,
		&this->shared_gl_context_native_window.get()
	);

	auto ruis_context = utki::make_shared<ruis::context>(ruis::context::parameters{
		.post_to_ui_thread_function =
			[this](std::function<void()> a) {
				NSEvent* e =
					[NSEvent otherEventWithType:NSEventTypeApplicationDefined
									   location:NSMakePoint(0, 0)
								  modifierFlags:0
									  timestamp:0
								   windowNumber:0
										context:nil
										subtype:0
										  data1:reinterpret_cast<NSInteger>(new std::function<void()>(std::move(a)))
										  data2:0];

				[this->macos_application.application postEvent:e atStart:NO];
			},
		.updater = this->updater,
		.renderer = utki::make_shared<ruis::render::renderer>(
			utki::make_shared<ruis::render::opengl::context>(ruis_native_window), //
			this->common_shaders,
			this->common_render_objects
		),
		.style_provider = this->ruis_style_provider,
		.units = ruis::units(
			get_dots_per_inch(), //
			get_dots_per_pp()
		)
	});

	utki::logcat_debug("application_glue::make_window(): ruis context created", '\n');

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

	utki::logcat_debug("application_glue::make_window(): inserting window to set", '\n');

	auto res = this->windows.insert(std::move(ruisapp_window));
	utki::assert(res.second, SL);

	utki::logcat_debug("application_glue::make_window(): window created", '\n');

	return res.first->get();
}

void application_glue::destroy_window(app_window& w)
{
	auto i = std::find(this->windows.begin(), this->windows.end(), w);
	utki::assert(i != this->windows.end(), SL);

	// Defer actual window object destruction until next main loop cycle,
	// for that put the window to the list of windows to destroy.
	this->windows_to_destroy.push_back(std::move(*i));

	this->windows.erase(i);
}

void application_glue::render()
{
	for (auto& w : this->windows) {
		w.get().render();
	}
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
	auto& glue = get_glue();
	glue.quit_flag.store(true);
}

ruisapp::window& ruisapp::application::make_window(window_parameters window_params)
{
	auto& glue = get_glue(*this);
	return glue.make_window(std::move(window_params));
}

void ruisapp::application::destroy_window(ruisapp::window& w)
{
	auto& glue = get_glue(*this);

	utki::assert(dynamic_cast<app_window*>(&w), SL);
	glue.destroy_window(static_cast<app_window&>(w));
}
