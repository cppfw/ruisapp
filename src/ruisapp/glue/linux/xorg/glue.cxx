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

#include <array>
#include <string_view>
#include <vector>

#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <utki/unicode.hpp>

#ifdef RUISAPP_RENDER_OPENGL
#	include <ruis/render/opengl/context.hpp>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <ruis/render/opengles/context.hpp>

#else
#	error "Unknown graphics API"
#endif

#include "../../../application.hpp"
#include "../../unix_common.hxx"

#include "cursor.hxx"
#include "display.hxx"
#include "key_code_map.hxx"
#include "window.hxx"

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace ruisapp;

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
// TODO: rename to application_glue
class os_platform_glue : public utki::destructable
{
public:
	const utki::shared_ref<display_wrapper> display = utki::make_shared<display_wrapper>();

private:
	utki::version_duplet gl_version;

	utki::shared_ref<native_window> shared_gl_context_native_window;
	utki::shared_ref<ruis::render::context> resource_loader_ruis_rendering_context;
	utki::shared_ref<const ruis::render::context::shaders> common_shaders;
	utki::shared_ref<const ruis::render::renderer::objects> common_render_objects;
	utki::shared_ref<ruis::resource_loader> ruis_resource_loader;
	utki::shared_ref<ruis::style_provider> ruis_style_provider;

	std::map<
		native_window::window_id_type, //
		utki::shared_ref<app_window> //
		>
		windows;

public:
	std::vector<utki::shared_ref<app_window>> windows_to_destroy;

	os_platform_glue(const utki::version_duplet& gl_version) :
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
				nullptr
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

	nitki::queue ui_queue;

	std::atomic_bool quit_flag = false;

	utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

	app_window& make_window(ruisapp::window_parameters window_params)
	{
		auto ruis_native_window = utki::make_shared<native_window>(
			this->display, //
			this->gl_version,
			window_params,
			&this->shared_gl_context_native_window.get()
		);

		auto ruis_context = utki::make_shared<ruis::context>(ruis::context::parameters{
			.post_to_ui_thread_function =
				[this](std::function<void()> proc) {
					this->ui_queue.push_back(std::move(proc));
				},
			.updater = this->updater,
			.renderer = utki::make_shared<ruis::render::renderer>(
#ifdef RUISAPP_RENDER_OPENGL
				utki::make_shared<ruis::render::opengl::context>(ruis_native_window),
#elif defined(RUISAPP_RENDER_OPENGLES)
				utki::make_shared<ruis::render::opengles::context>(ruis_native_window),
#else
#	error "Unknown graphics API"
#endif
				this->common_shaders,
				this->common_render_objects
			),
			.style_provider = this->ruis_style_provider,
			.units =
				[this]() {
					return ruis::units(
						this->display.get().get_dots_per_inch(), //
						this->display.get().get_dots_per_pp()
					);
				}()
		});

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

		auto res = this->windows.insert( //
			std::make_pair(
				ruisapp_window.get().ruis_native_window.get().get_id(), //
				std::move(ruisapp_window)
			)
		);
		utki::assert(res.second, SL);

		return res.first->second.get();
	}

	void destroy_window(app_window& w)
	{
		auto i = this->windows.find(w.ruis_native_window.get().get_id());
		utki::assert(i != this->windows.end(), SL);

		// Defer actual window object destruction until next main loop cycle,
		// for that put the window to the list of windows to destroy.
		this->windows_to_destroy.push_back(i->second);

		this->windows.erase(i);
	}

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

	void apply_new_win_dims()
	{
		for (auto& win : this->windows) {
			auto& w = win.second.get();
			if (w.new_win_dims.is_positive_or_zero()) {
				w.gui.set_viewport(ruis::rect(0, w.new_win_dims));
			}
			w.new_win_dims = {-1, -1};
		}
	}
};
} // namespace

namespace {
os_platform_glue& get_glue(ruisapp::application& app)
{
	return static_cast<os_platform_glue&>(app.pimpl.get());
}
} // namespace

application::application(parameters params) :
	application(
		utki::make_unique<os_platform_glue>(params.graphics_api_version), //
		get_application_directories(params.name),
		std::move(params)
	)
{}

namespace {

class xevent_waitable : public opros::waitable
{
public:
	xevent_waitable(Display* d) :
		opros::waitable(XConnectionNumber(d))
	{}
};

ruis::mouse_button button_number_to_enum(unsigned number)
{
	switch (number) {
		case 1: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::left;
		default:
		case 2: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::middle;
		case 3: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::right;
		case 4: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_up;
		case 5: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_down;
		case 6: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_left;
		case 7: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::wheel_right;
	}
}

class key_event_unicode_provider : public ruis::gui::input_string_provider
{
	const native_window& window;
	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	XKeyEvent& event;

public:
	key_event_unicode_provider(
		const native_window& window, //
		XKeyEvent& event
	) :
		window(window),
		event(event)
	{}

	std::u32string get() const override
	{
		return this->window.get_string(this->event);
	}
};

} // namespace

void application::quit() noexcept
{
	auto& glue = get_glue(*this);
	glue.quit_flag.store(true);
}

ruisapp::window& application::make_window(window_parameters window_params)
{
	auto& glue = get_glue(*this);
	return glue.make_window(std::move(window_params));
}

void application::destroy_window(ruisapp::window& w)
{
	auto& glue = get_glue(*this);

	utki::assert(dynamic_cast<app_window*>(&w), SL);
	glue.destroy_window(static_cast<app_window&>(w));
}

int main(int argc, const char** argv)
{
	auto app = ruisapp::application_factory::make_application(argc, argv);
	if (!app) {
		// Not an error. The app just did not show any GUI to the user.
		return 0;
	}
	utki::assert(app, SL);

	auto& glue = get_glue(*app);

	xevent_waitable xew(glue.display.get().xorg_display.display);

	opros::wait_set wait_set(2);

	wait_set.add(xew, {opros::ready::read}, &xew);
	utki::scope_exit xew_wait_set_scope_exit([&]() {
		wait_set.remove(xew);
	});

	wait_set.add(glue.ui_queue, {opros::ready::read}, &glue.ui_queue);
	utki::scope_exit ui_queue_wait_set_scope_exit([&]() {
		wait_set.remove(glue.ui_queue);
	});

	while (!glue.quit_flag.load()) {
		glue.windows_to_destroy.clear();

		// main loop cycle sequence as required by ruis:
		// - update updateables
		// - render
		// - wait for events and handle them

		auto to_wait_ms = glue.updater.get().update();
		glue.render();
		wait_set.wait(to_wait_ms);

		auto triggered_events = wait_set.get_triggered();

		bool ui_queue_ready_to_read = false;

		for (auto& ei : triggered_events) {
			if (ei.user_data == &glue.ui_queue) {
				ui_queue_ready_to_read = true;
			}
		}

		if (ui_queue_ready_to_read) {
			while (auto m = glue.ui_queue.pop_front()) {
				utki::log_debug([](auto& o) {
					o << "loop message" << std::endl;
				});
				m();
			}
		}

		// NOTE: do not check 'read' flag for X event, for some reason when waiting
		//       with 0 timeout it will never be set.
		//       Maybe some bug in XWindows.
		while (XPending(glue.display.get().xorg_display.display) > 0) {
			XEvent event;
			XNextEvent(
				glue.display.get().xorg_display.display, //
				&event
			);

			// get the window the event is sent to
			auto window = glue.get_window(event.xany.window);
			if (!window) {
				continue;
			}

			auto& w = *window;

			switch (event.type) {
				case Expose:
					if (event.xexpose.count != 0) {
						break;
					}
					// TODO: instead of rendering, set render needed for this window, when render only if needed is implemented
					w.render();
					break;
				case ConfigureNotify:
					// squash all window resize events into one, for that store the new
					// window dimensions and update the viewport later only once
					w.new_win_dims.x() = ruis::real(event.xconfigure.width);
					w.new_win_dims.y() = ruis::real(event.xconfigure.height);
					break;
				case KeyPress:
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						ruis::key key = key_code_map[std::uint8_t(event.xkey.keycode)];

						w.gui.send_key(
							true, //
							key
						);

						key_event_unicode_provider string_provider(
							w.ruis_native_window, //
							event.xkey
						);

						w.gui.send_character_input(
							string_provider, //
							key
						);
					}
					break;
				case KeyRelease:
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						ruis::key key = key_code_map[std::uint8_t(event.xkey.keycode)];

						// detect auto-repeated key events
						if (XEventsQueued(
								glue.display.get().xorg_display.display, //
								QueuedAfterReading
							))
						{
							// there are other events queued

							XEvent nev;
							XPeekEvent(
								glue.display.get().xorg_display.display, //
								&nev
							);

							if (nev.type == KeyPress && nev.xkey.time == event.xkey.time &&
								nev.xkey.keycode == event.xkey.keycode)
							{
								// key wasn't actually released
								w.gui.send_character_input(
									key_event_unicode_provider(w.ruis_native_window, nev.xkey), //
									key
								);

								// remove the key down event from queue
								XNextEvent(glue.display.get().xorg_display.display, &nev);
								break;
							}
						}

						w.gui.send_key(false, key);
					}
					break;
				case ButtonPress:
					w.gui.send_mouse_button(
						true, //
						ruis::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case ButtonRelease:
					w.gui.send_mouse_button(
						false, //
						ruis::vector2(event.xbutton.x, event.xbutton.y),
						button_number_to_enum(event.xbutton.button),
						0
					);
					break;
				case MotionNotify:
					w.gui.send_mouse_move(
						ruis::vector2(
							event.xmotion.x, //
							event.xmotion.y
						), //
						0
					);
					break;
				case EnterNotify:
					w.gui.send_mouse_hover(
						true, //
						0
					);
					break;
				case LeaveNotify:
					w.gui.send_mouse_hover(
						false, //
						0
					);
					break;
				case ClientMessage:
					// probably a WM_DELETE_WINDOW event
					{
						// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
						char* name = XGetAtomName(
							glue.display.get().xorg_display.display, //
							event.xclient.message_type
						);
						if ("WM_PROTOCOLS"sv == name) {
							auto& nw = w.ruis_native_window.get();
							if (nw.close_handler) {
								nw.close_handler();
							}
						}
						XFree(name);
					}
					break;
				default:
					// ignore
					break;
			}
		}

		glue.apply_new_win_dims();
	}

	return 0;
}
