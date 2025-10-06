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

#ifdef RUISAPP_RENDER_OPENGL
// #	include <GL/glew.h>
#	include <ruis/render/opengl/context.hpp>
#elif defined(RUISAPP_RENDER_OPENGLES)
// #	include <GLES2/gl2.h>
#	include <ruis/render/opengles/context.hpp>
#else
#	error "Unknown graphics API"
#endif

using namespace std::string_view_literals;

application_glue::application_glue(const utki::version_duplet& gl_version) :
	gl_version(gl_version)
#if CFG_OS_NAME != CFG_OS_NAME_EMSCRIPTEN
	,
	shared_gl_context_native_window( //
		utki::make_shared<native_window>(
			this->display, //
			this->gl_version,
			ruisapp::window_parameters{
				.dims = {1, 1},
				.title = {},
				.fullscreen = false
},
			nullptr // no shared gl context
		)
	),
	resource_loader_ruis_rendering_context(
#	ifdef RUISAPP_RENDER_OPENGL
		utki::make_shared<ruis::render::opengl::context>(this->shared_gl_context_native_window)
#	elif defined(RUISAPP_RENDER_OPENGLES)
		utki::make_shared<ruis::render::opengles::context>(this->shared_gl_context_native_window)
#	else
#		error "Unknown graphics API"
#	endif
	),
	common_shaders( //
		[&]() {
			utki::assert(this->resource_loader_ruis_rendering_context.to_shared_ptr(), SL);
			return this->resource_loader_ruis_rendering_context.get().make_shaders();
		}()
	),
	common_render_objects( //
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
#endif
{}

app_window* application_glue::get_window(native_window::window_id_type id)
{
	auto i = this->windows.find(id);
	if (i == this->windows.end()) {
		return nullptr;
	}
	return &i->second.get();
}

void application_glue::render()
{
	for (auto& w : this->windows) {
		w.second.get().render();
	}
}

void application_glue::apply_new_win_dims()
{
	for (auto& win : this->windows) {
		auto& w = win.second.get();
		if (w.new_win_dims.is_positive_or_zero()) {
			w.gui.set_viewport(ruis::rect(0, w.new_win_dims));
		}
		w.new_win_dims = {-1, -1};
	}
}

ruisapp::window& application_glue::make_window(ruisapp::window_parameters window_params)
{
	auto ruis_native_window = utki::make_shared<native_window>(
		this->display,
		this->gl_version,
		window_params,
#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
		nullptr
#else
		&this->shared_gl_context_native_window.get()
#endif
	);

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
	auto rendering_context = utki::make_shared<ruis::render::opengles::context>(ruis_native_window);

	auto common_render_objects = utki::make_shared<ruis::render::renderer::objects>(rendering_context);
	auto common_shaders = rendering_context.get().make_shaders();

	auto ruis_resource_loader = utki::make_shared<ruis::resource_loader>(
		rendering_context, //
		common_render_objects
	);

	auto ruis_style_provider = utki::make_shared<ruis::style_provider>(std::move(ruis_resource_loader));
#endif

	auto ruis_context = utki::make_shared<ruis::context>(ruis::context::parameters{
		.post_to_ui_thread_function =
			[display = this->display](std::function<void()> procedure) {
				SDL_Event e;
				SDL_memset(&e, 0, sizeof(e));
				e.type = display.get().user_event_type_id;
				e.user.code = 0;
				// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
				e.user.data1 = new std::function<void()>(std::move(procedure));
				e.user.data2 = nullptr;
				SDL_PushEvent(&e);
			},
		.updater = this->updater,
		.renderer =
#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
			utki::make_shared<ruis::render::renderer>(
				std::move(rendering_context),
				std::move(common_shaders),
				std::move(common_render_objects)
			),
#else
			utki::make_shared<ruis::render::renderer>(
#	ifdef RUISAPP_RENDER_OPENGL
				utki::make_shared<ruis::render::opengl::context>(ruis_native_window),
#	elif defined(RUISAPP_RENDER_OPENGLES)
				utki::make_shared<ruis::render::opengles::context>(ruis_native_window),
#	else
#		error "Unknown graphics API"
#	endif
				this->common_shaders,
				this->common_render_objects
			),
#endif
		.style_provider =
#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
			std::move(ruis_style_provider),
#else
			this->ruis_style_provider,
#endif
		.units = ruis::units(
			ruis_native_window.get().get_dpi(), //
			ruis_native_window.get().get_scale_factor()
		)
	});

	auto ruisapp_window = utki::make_shared<app_window>(
		std::move(ruis_context), //
		std::move(ruis_native_window)
	);

	ruisapp_window.get().gui.set_viewport( //
		ruis::rect({0, 0}, ruisapp_window.get().ruis_native_window.get().get_dims())
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

void application_glue::destroy_window(native_window::window_id_type id)
{
	auto i = this->windows.find(id);
	utki::assert(i != this->windows.end(), SL);

	this->windows_to_destroy.push_back(std::move(i->second));
	this->windows.erase(i);
}

namespace {
ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	char* base_dir = SDL_GetPrefPath("", std::string(app_name).c_str());
	utki::scope_exit base_dir_scope_exit([&]() {
		SDL_free(base_dir);
	});

	ruisapp::application::directories dirs;

	dirs.cache = utki::cat(base_dir, "cache/"sv);
	dirs.config = utki::cat(base_dir, "config/"sv);
	dirs.state = utki::cat(base_dir, "state/"sv);

	// std::cout << "cache dir = " << dirs.cache << std::endl;
	// std::cout << "config dir = " << dirs.config << std::endl;
	// std::cout << "state dir = " << dirs.state << std::endl;

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

void ruisapp::application::quit() noexcept
{
	auto& glue = get_glue(*this);

	// TODO: send SDL_QUIT event instead of setting the flag?
	glue.quit_flag.store(true);
}

ruisapp::window& ruisapp::application::make_window(ruisapp::window_parameters window_params)
{
	auto& glue = get_glue(*this);

#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
	if (glue.get_num_windows() == 1) {
		throw std::logic_error(
			"ruisapp::application::make_window(): one window already exists. Only one window is allowed on emscripten."
		);
	}
#endif

	return glue.make_window(std::move(window_params));
}

void ruisapp::application::destroy_window(ruisapp::window& w)
{
#if CFG_OS_NAME == CFG_OS_NAME_EMSCRIPTEN
	throw std::logic_error(
		"ruisapp::application::destroy_window(): programmatically destroying window on emscripten is not allowed. The window is destroyed along with the browser window/tab."
	);
#else
	auto& glue = get_glue(*this);

	utki::assert(dynamic_cast<app_window*>(&w), SL);
	auto& app_win =
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast, "assert(dynamic_cast) done")
		static_cast<app_window&>(w);
	glue.destroy_window(app_win.ruis_native_window.get().get_id());
#endif
}
