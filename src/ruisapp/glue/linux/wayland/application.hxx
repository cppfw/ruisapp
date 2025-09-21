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
	bool outputs_changed_message_pending = false;

public:
	// keep track of window state as notified by Wayland
	struct window_state {
		bool fullscreen = false;
		bool activated = false;
	} actual_state;

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

	~app_window()
	{
		// If we have pending frame callback, then destroy it to in order to cancel the callback.
		// This is to avoid the callback being called after the window object has been destroyed.
		if (this->frame_callback) {
			wl_callback_destroy(this->frame_callback);
		}
	}

	void resize(const r4::vector2<uint32_t>& dims);

	void refresh_dimensions();

	void notify_outputs_changed();

	void schedule_rendering();

private:
	wl_callback* frame_callback = nullptr;

	static void wl_surface_frame_done(
		void* data, //
		struct wl_callback* callback,
		uint32_t time
	);

	static const constexpr wl_callback_listener wl_surface_frame_listener = {.done = &wl_surface_frame_done};
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

	// TODO: make windowless shared egl context
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
	std::vector<utki::shared_ref<app_window>> windows_to_destroy;

	native_window::window_id_type get_shared_gl_context_window_id() const noexcept
	{
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

	ruisapp::window& make_window(ruisapp::window_parameters window_params);
	void destroy_window(app_window& w);

	app_window* get_window(native_window::window_id_type id)
	{
		auto i = this->windows.find(id);
		if (i == this->windows.end()) {
			return nullptr;
		}
		return &i->second.get();
	}

	// render all windows if needed
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
