#pragma once

#include <atomic>
#include <map>

#include <nitki/queue.hpp>

#include "../../../application.hpp"
#include "../../../window.hpp"
#include "../../unix_common.hxx"

#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
	// keep track of current window dimensions
	r4::vector2<uint32_t> cur_window_dims;

	bool outputs_changed_message_pending = false;

public:
	// keeps track of window fullscreen state as reported by wayland
	bool is_actually_fullscreen = false;

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

	void resize(const r4::vector2<uint32_t>& dims);

	void notify_outputs_changed();
};
} // namespace

namespace {
class application_glue : public utki::destructable
{
public:
	const utki::shared_ref<display_wrapper> display = utki::make_shared<display_wrapper>();

	std::atomic_bool quit_flag = false;

	nitki::queue ui_queue;

	class wayland_waitable : public opros::waitable
	{
	public:
		wayland_waitable(wayland_display_wrapper& wayland_display) :
			opros::waitable([&]() {
				auto fd = wl_display_get_fd(wayland_display.display);
				utki::assert(fd != 0, SL);
				return fd;
			}())
		{}
	} waitable;

	utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

	const utki::version_duplet gl_version;

	// TODO: make windowless sahred egl context
	const utki::shared_ref<native_window> shared_gl_context_native_window;
	const utki::shared_ref<ruis::render::context> resource_loader_ruis_rendering_context;
	const utki::shared_ref<const ruis::render::context::shaders> common_shaders;
	const utki::shared_ref<const ruis::render::renderer::objects> common_render_objects;
	const utki::shared_ref<ruis::resource_loader> ruis_resource_loader;
	const utki::shared_ref<ruis::style_provider> ruis_style_provider;

private:
	std::map<
		native_window::window_id_type, //
		utki::shared_ref<app_window> //
		>
		windows;

public:
	native_window::window_id_type get_shared_gl_context_window_id()const noexcept{
		return this->shared_gl_context_native_window.get().get_id();
	}

	application_glue(const utki::version_duplet& gl_version) :
		waitable(this->display.get().wayland_display),
		gl_version(gl_version),
		shared_gl_context_native_window( //
			utki::make_shared<native_window>(
				this->display, //
				this->gl_version,
				ruisapp::window_parameters{
					.dims = {1, 1},
					.title = {},
					.fullscreen = false,
					.visible = false
    },
				nullptr // no shared gl context
			)
		),
		resource_loader_ruis_rendering_context(
#ifdef RUISAPP_RENDER_OPENGL
			utki::make_shared<ruis::render::opengl::context>(this->shared_gl_context_native_window)
#elif defined(RUISAPP_RENDER_OPENGLES)
			utki::make_shared<ruis::render::opengles::context>(this->shared_gl_context_native_window)
#else
#	error "Unknown graphics API"
#endif
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
	{}

	ruisapp::window& make_window(const ruisapp::window_parameters& window_params);
	void destroy_window(app_window& w);

	app_window* get_window(native_window::window_id_type id)
	{
		auto i = this->windows.find(id);
		if (i == this->windows.end()) {
			return nullptr;
		}
		return &i->second.get();
	}

	void render()
	{
		for (const auto& w : this->windows) {
			w.second.get().render();
		}
	}
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
