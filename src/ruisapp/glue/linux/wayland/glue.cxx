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

#include <atomic>
#include <optional>

#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <papki/fs_file.hpp>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glew.h>
#	include <ruis/render/opengl/context.hpp>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <GLES2/gl2.h>
#	include <ruis/render/opengles/context.hpp>

#else
#	error "Unknown graphics API"
#endif

#include "../../../application.hpp"
#include "../../unix_common.hxx"

#include "display.hxx"

using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
class os_platform_glue : public utki::destructable
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

private:
	utki::version_duplet gl_version;

public:
	os_platform_glue(const utki::version_duplet& gl_version) :
		waitable(this->display.get().wayland_display),
		gl_version(gl_version)
	{}
};
} // namespace

namespace {
os_platform_glue& get_glue(ruisapp::application& app)
{
	return static_cast<os_platform_glue&>(app.pimpl.get());
}
} // namespace

namespace {
// TODO: why is this needed?
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool application_constructed = false;
} // namespace

application::application(parameters params) :
	application(
		utki::make_unique<os_platform_glue>(params.graphics_api_version), //
		get_application_directories(params.name),
		std::move(params)
	)
{}

// application::application(std::string name, const window_parameters& wp) :
// 	name(std::move(name)),
// 	window_pimpl(std::make_unique<window_wrapper>(wp)),
// 	gui(utki::make_shared<ruis::context>(
// 		utki::make_shared<ruis::style_provider>( //
// 			utki::make_shared<ruis::resource_loader>( //
// 				utki::make_shared<ruis::render::renderer>(
// #ifdef RUISAPP_RENDER_OPENGL
// 					utki::make_shared<ruis::render::opengl::context>()
// #elif defined(RUISAPP_RENDER_OPENGLES)
// 					utki::make_shared<ruis::render::opengles::context>()
// #else
// #	error "Unknown graphics API"
// #endif
// 				)
// 			)
// 		),
// 		utki::make_shared<ruis::updater>(),
// 		ruis::context::parameters{
// 			.post_to_ui_thread_function =
// 				[this](std::function<void()> proc) {
// 					get_impl(*this).ui_queue.push_back(std::move(proc));
// 				},
// 			.set_mouse_cursor_function =
// 				[this](ruis::mouse_cursor c) {
// 					auto& ww = get_impl(*this);
// 					ww.seat.pointer.set_cursor(c);
// 				}
// 		}
// 	)),
// 	directory(get_application_directories(this->name))
// {
// 	this->update_window_rect(ruis::rect(
// 		0, //
// 		0,
// 		ruis::real(wp.dims.x()),
// 		ruis::real(wp.dims.y())
// 	));

// 	application_constructed = true;
// }

// TODO:
// void application::set_mouse_cursor_visible(bool visible)
// {
// 	auto& ww = get_impl(this->window_pimpl);
// 	ww.seat.pointer.set_cursor_visible(visible);
// }

// TODO:
// void application::set_fullscreen(bool fullscreen)
// {
// 	if (fullscreen == this->is_fullscreen_v) {
// 		return;
// 	}

// 	this->is_fullscreen_v = fullscreen;

// 	auto& ww = get_impl(this->window_pimpl);

// 	utki::log_debug([&](auto& o) {
// 		o << "set_fullscreen(" << fullscreen << ")" << std::endl;
// 	});

// 	if (fullscreen) {
// 		ww.pre_fullscreen_win_dims = ww.cur_window_dims;
// 		utki::log_debug([&](auto& o) {
// 			o << " old win dims = " << std::dec << ww.pre_fullscreen_win_dims << std::endl;
// 		});
// 		xdg_toplevel_set_fullscreen(ww.toplevel.toplev, nullptr);
// 	} else {
// 		xdg_toplevel_unset_fullscreen(ww.toplevel.toplev);
// 	}
// }

// TODO:
// void ruisapp::application::quit() noexcept
// {
// 	auto& ww = get_impl(this->window_pimpl);
// 	ww.quit_flag.store(true);
// }

// NOLINTNEXTLINE(bugprone-exception-escape, "it's what we want")
int main(int argc, const char** argv)
{
	auto application = ruisapp::application_factory::make_application(argc, argv);
	if (!application) {
		// Not an error. The app just did not show any GUI to the user.
		return 0;
	}

	auto& app = *application;

	auto& glue = get_glue(app);

	opros::wait_set wait_set(2);
	wait_set.add(
		glue.waitable, //
		{opros::ready::read},
		&glue.waitable
	);
	wait_set.add(
		glue.ui_queue, //
		{opros::ready::read},
		&glue.ui_queue
	);

	utki::scope_exit scope_exit_wait_set([&]() noexcept {
		wait_set.remove(glue.ui_queue);
		wait_set.remove(glue.waitable);
	});

	while (!glue.quit_flag.load()) {
		// std::cout << "loop" << std::endl;

		// sequence:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle
		auto to_wait_ms = app.gui.update();
		glue.render();

		auto& disp = glue.display.get().wayland_display.display;

		// prepare wayland queue for waiting for events
		while (wl_display_prepare_read(disp) != 0) {
			// there are events in wayland queue, dispatch them, as we need empty queue
			// when we start waiting for events on the queue
			if (wl_display_dispatch_pending(disp) < 0) {
				throw std::runtime_error(utki::cat(
					"wl_display_dispatch_pending() failed: ", //
					strerror(errno)
				));
			}
		}

		{
			utki::scope_exit scope_exit_wayland_prepare_read([&]() {
				wl_display_cancel_read(disp);
			});

			// send queued wayland requests to server
			if (wl_display_flush(disp) < 0) {
				if (errno == EAGAIN) {
					// std::cout << "wayland display more to flush" << std::endl;
					wait_set.change(
						glue.waitable, //
						{opros::ready::read, opros::ready::write},
						&glue.waitable
					);
				} else {
					throw std::runtime_error(utki::cat(
						"wl_display_flush() failed: ", //
						strerror(errno)
					));
				}
			} else {
				// std::cout << "wayland display flushed" << std::endl;
				wait_set.change(
					glue.waitable, //
					{opros::ready::read},
					&glue.waitable
				);
			}

			// std::cout << "wait for " << to_wait_ms << "ms" << std::endl;

			wait_set.wait(to_wait_ms);

			// std::cout << "waited" << std::endl;

			auto triggered_events = wait_set.get_triggered();

			// std::cout << "num triggered = " << triggered_events.size() << std::endl;

			// we want to first handle messages of ui queue,
			// but since we don't know the order of triggered objects,
			// first go through all of them and set readiness flags
			bool ui_queue_ready_to_read = false;
			bool wayland_queue_ready_to_read = false;

			for (auto& ei : triggered_events) {
				if (ei.user_data == &glue.ui_queue) {
					if (ei.flags.get(opros::ready::error)) {
						throw std::runtime_error("waiting on ui queue errored");
					}
					if (ei.flags.get(opros::ready::read)) {
						// std::cout << "ui queue ready" << std::endl;
						ui_queue_ready_to_read = true;
					}
				} else if (ei.user_data == &glue.waitable) {
					if (ei.flags.get(opros::ready::error)) {
						throw std::runtime_error("waiting on wayland file descriptor errored");
					}
					if (ei.flags.get(opros::ready::read)) {
						// std::cout << "wayland queue ready to read" << std::endl;
						wayland_queue_ready_to_read = true;
					}
				}
			}

			if (ui_queue_ready_to_read) {
				while (auto m = glue.ui_queue.pop_front()) {
					utki::log_debug([](auto& o) {
						o << "loop proc" << std::endl;
					});
					m();
				}
			}

			if (wayland_queue_ready_to_read) {
				scope_exit_wayland_prepare_read.release();

				// std::cout << "read" << std::endl;
				if (wl_display_read_events(disp) < 0) {
					throw std::runtime_error(utki::cat(
						"wl_display_read_events() failed: ", //
						strerror(errno)
					));
				}

				// std::cout << "disppatch" << std::endl;
				if (wl_display_dispatch_pending(disp) < 0) {
					throw std::runtime_error(utki::cat(
						"wl_display_dispatch_pending() failed: ", //
						strerror(errno)
					));
				}
			}
		}
	}

	return 0;
}

void wayland_output_wrapper::wl_output_geometry(
	void* data,
	struct wl_output* wl_output,
	int32_t x,
	int32_t y,
	int32_t physical_width,
	int32_t physical_height,
	int32_t subpixel,
	const char* make,
	const char* model,
	int32_t transform
)
{
	ASSERT(data)
	auto& self = *static_cast<wayland_output_wrapper*>(data);

	self.position = {uint32_t(x), uint32_t(y)};
	self.physical_size_mm = {uint32_t(physical_width), uint32_t(physical_height)};

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ")" << '\n' //
		  << "  physical_size_mm = " << self.physical_size_mm << '\n' //
		  << "  make = " << make << '\n' //
		  << "  model = " << model << std::endl;
	});

	if (!application_constructed) {
		// unable to obtain window_wrapper object before application is constructed,
		// cannot do more without window_wrapper object
		utki::log_debug([](auto& o) {
			o << "  called within application constructor" << std::endl;
		});
		return;
	}

	auto& ww = get_impl(ruisapp::application::inst());
	ww.notify_outputs_changed();
}

void wayland_output_wrapper::wl_output_mode(
	void* data,
	struct wl_output* wl_output,
	uint32_t flags,
	int32_t width,
	int32_t height,
	int32_t refresh
)
{
	ASSERT(data)
	auto& self = *static_cast<output_wrapper*>(data);

	self.resolution = {uint32_t(width), uint32_t(height)};

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ") resolution = " << self.resolution << std::endl;
	});

	if (!application_constructed) {
		// unable to obtain window_wrapper object before application is constructed,
		// cannot do more without window_wrapper object
		utki::log_debug([](auto& o) {
			o << "  called within application constructor" << std::endl;
		});
		return;
	}

	auto& ww = get_impl(ruisapp::application::inst());
	ww.notify_outputs_changed();
}

void wayland_output_wrapper::wl_output_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
	ASSERT(data)
	auto& self = *static_cast<output_wrapper*>(data);

	self.scale = uint32_t(std::max(factor, 1));

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ") scale = " << self.scale << std::endl;
	});

	if (!application_constructed) {
		// unable to obtain window_wrapper object before application is constructed,
		// cannot do more without window_wrapper object
		utki::log_debug([](auto& o) {
			o << "  called within application constructor" << std::endl;
		});
		return;
	}

	auto& ww = get_impl(ruisapp::application::inst());
	ww.notify_outputs_changed();
}

void wayland_surface_wrapper::wl_surface_enter(void* data, wl_surface* surface, wl_output* output)
{
	utki::log_debug([&](auto& o) {
		o << "surface enters output(" << get_output_id(output) << ")" << std::endl;
	});

	ASSERT(data)
	auto& self = *static_cast<surface_wrapper*>(data);

	ASSERT(self.outputs.find(output) == self.outputs.end())

	self.outputs.insert(output);

	auto& ww = get_impl(ruisapp::application::inst());
	ww.notify_outputs_changed();
}

void wayland_surface_wrapper::wl_surface_leave(void* data, wl_surface* surface, wl_output* output)
{
	utki::log_debug([&](auto& o) {
		o << "surface leaves output(" << get_output_id(output) << ")" << std::endl;
	});

	ASSERT(data)
	auto& self = *static_cast<surface_wrapper*>(data);

	ASSERT(self.outputs.find(output) != self.outputs.end())

	self.outputs.erase(output);

	auto& ww = get_impl(ruisapp::application::inst());
	ww.notify_outputs_changed();
}

void wayland_pointer_wrapper::wl_pointer_motion(
	void* data,
	wl_pointer* pointer,
	uint32_t time,
	wl_fixed_t x,
	wl_fixed_t y
)
{
	ASSERT(data)
	auto& self = *static_cast<pointer_wrapper*>(data);

	auto& ww = get_impl(ruisapp::application::inst());

	ruis::vector2 pos( //
		ruis::real(wl_fixed_to_double(x)),
		ruis::real(wl_fixed_to_double(y))
	);
	pos *= ww.scale;
	self.cur_pointer_pos = round(pos);

	// std::cout << "mouse move: x,y = " << std::dec << self.cur_pointer_pos << std::endl;
	handle_mouse_move( //
		ruisapp::application::inst(),
		self.cur_pointer_pos,
		0
	);
}

void wayland_touch_wrapper::wl_touch_down( //
	void* data,
	wl_touch* touch,
	uint32_t serial,
	uint32_t time,
	wl_surface* surface,
	int32_t id,
	wl_fixed_t x,
	wl_fixed_t y
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch down event" << std::endl;
	});

	auto& ww = get_impl(ruisapp::application::inst());

	if (ww.surface.sur != surface) {
		utki::log_debug([](auto& o) {
			o << "  non-window surface touched, ignore" << std::endl;
		});
		return;
	}

	ASSERT(data)
	auto& self = *static_cast<touch_wrapper*>(data);
	ASSERT(self.touch == touch)

	ASSERT(!utki::contains(self.touch_points, id))

	ruis::vector2 pos( //
		ruis::real(wl_fixed_to_double(x)),
		ruis::real(wl_fixed_to_double(y))
	);
	pos = round(pos * ww.scale);

	auto insert_result = self.touch_points.insert(std::make_pair(
		id,
		touch_point{
			.ruis_id = unsigned(id) + 1, // id = 0 reserved for mouse
			.pos = pos
		}
	));
	ASSERT(insert_result.second) // pair successfully inserted

	const touch_point& tp = insert_result.first->second;

	handle_mouse_button(
		ruisapp::application::inst(),
		true, // is_down
		tp.pos,
		ruis::mouse_button::left,
		tp.ruis_id
	);
}

void wayland_touch_wrapper::wl_touch_up( //
	void* data,
	wl_touch* touch,
	uint32_t serial,
	uint32_t time,
	int32_t id
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch up event" << std::endl;
	});

	ASSERT(data)
	auto& self = *static_cast<touch_wrapper*>(data);
	ASSERT(self.touch == touch)

	auto i = self.touch_points.find(id);
	ASSERT(i != self.touch_points.end())

	const touch_point& tp = i->second;

	handle_mouse_button(
		ruisapp::application::inst(),
		false, // is_down
		tp.pos,
		ruis::mouse_button::left,
		tp.ruis_id
	);

	self.touch_points.erase(i);
}

void wayland_touch_wrapper::wl_touch_motion( //
	void* data,
	wl_touch* touch,
	uint32_t time,
	int32_t id,
	wl_fixed_t x,
	wl_fixed_t y
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch motion event" << std::endl;
	});

	auto& ww = get_impl(ruisapp::application::inst());

	ASSERT(data)
	auto& self = *static_cast<touch_wrapper*>(data);
	ASSERT(self.touch == touch)

	auto i = self.touch_points.find(id);
	ASSERT(i != self.touch_points.end())

	touch_point& tp = i->second;

	ruis::vector2 pos( //
		ruis::real(wl_fixed_to_double(x)),
		ruis::real(wl_fixed_to_double(y))
	);
	pos = round(pos * ww.scale);

	tp.pos = pos;

	handle_mouse_move( //
		ruisapp::application::inst(),
		tp.pos,
		tp.ruis_id
	);
}

// sent when compositor desides that touch gesture is going on, so
// all touch points become invalid and must be cancelled. No further
// events will be sent by wayland for current touch points.
void wayland_touch_wrapper::wl_touch_cancel( //
	void* data,
	wl_touch* touch
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch cancel event" << std::endl;
	});

	ASSERT(data)
	auto& self = *static_cast<wayland_touch_wrapper*>(data);
	ASSERT(self.touch == touch)

	// send out-of-window button-up events fro all touch points
	for (const auto& pair : self.touch_points) {
		const auto& tp = pair.second;

		handle_mouse_button(
			ruisapp::application::inst(),
			false, // is_down
			{-1, -1},
			ruis::mouse_button::left,
			tp.ruis_id
		);
	}

	self.touch_points.clear();
}
