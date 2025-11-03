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
	float scale = [[UIScreen mainScreen] scale];

	ruis::real value;

	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
		value = 132 * scale;
	} else if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
		value = 163 * scale;
	} else {
		value = 160 * scale;
	}
	utki::log_debug([&](auto& o) {
		o << "dpi = " << value << std::endl;
	});
	return value;
}
} // namespace

namespace {
ruis::real get_dots_per_pp()
{
	float scale = [[UIScreen mainScreen] scale];

	return ruis::real(scale);
}
} // namespace

application_glue::application_glue(utki::version_duplet gl_version) :
	gl_version(std::move(gl_version))
{}

void application_glue::render()
{
	if (this->window.has_value()) {
		this->window.value().render();
	}
}

app_window& application_glue::make_window(ruisapp::window_parameters window_params)
{
	if (this->window.has_value()) {
		throw std::logic_error("application::make_window(): one window already exists, only one window allowed on ios");
	}

	utki::assert(!this->window.has_value(), SL);

	auto ruis_native_window = utki::make_shared<native_window>(
		this->gl_version, //
		std::move(window_params)
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
				auto p = reinterpret_cast<NSInteger>(new std::function<void()>(std::move(procedure)));

				dispatch_async(dispatch_get_main_queue(), ^{
				  std::unique_ptr<std::function<void()>> m(reinterpret_cast<std::function<void()>*>(p));
				  (*m)();
				});
			},
		.updater = this->updater,
		.renderer = utki::make_shared<ruis::render::renderer>(
			std::move(rendering_context),
			std::move(common_shaders),
			std::move(common_render_objects)
		),
		.style_provider = std::move(ruis_style_provider),
		.units = ruis::units(
			get_dots_per_inch(), //
			get_dots_per_pp()
		)
	});

	this->window.emplace(
		std::move(ruis_context), //
		std::move(ruis_native_window)
	);

	// The GL viewport will be set later when ios window is displayed

	utki::assert(this->window.has_value(), SL);
	return this->window.value();
}

ruisapp::application::application(parameters params) :
	application(
		{utki::make_unique<application_glue>(params.graphics_api_version), //
		 {}, // TODO: set application directories
		 std::move(params)}
	)
{}

std::unique_ptr<fsif::file> ruisapp::application::get_res_file(std::string_view path) const
{
	std::string dir([[[NSBundle mainBundle] resourcePath] fileSystemRepresentation]);

	//	TRACE(<< "res path = " << dir << std::endl)

	auto rdf = std::make_unique<fsif::root_dir>(
		std::make_unique<fsif::native_file>(), //
		dir + "/" // TODO: use utki::cat()?
	);
	rdf->set_path(path);

	return rdf;
}

void ruisapp::application::quit() noexcept
{
	// TODO:
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
			o << "ruisapp::application::destroy_window(): programmatically destroying window on ios is not allowed. The window is destroyed along with the application.";
		},
		SL
	);
}
