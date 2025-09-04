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
#include <map>

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

#include "application.hxx"

// include implementations
#include "application.cxx"
#include "wayland_keyboard.cxx"
#include "wayland_output.cxx"
#include "wayland_pointer.cxx"
#include "wayland_surface.cxx"
#include "wayland_touch.cxx"
#include "window.cxx"
#include "xdg_toplevel.cxx"

using namespace std::string_view_literals;

using namespace ruisapp;

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
		auto to_wait_ms = glue.updater.get().update();
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

			// TODO: wayland queue is constantly ready to read. figure out why.
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
