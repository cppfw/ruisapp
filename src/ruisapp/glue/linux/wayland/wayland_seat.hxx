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

#pragma once

#include "wayland_compositor.hxx"
#include "wayland_keyboard.hxx"
#include "wayland_pointer.hxx"
#include "wayland_registry.hxx"
#include "wayland_shm.hxx"
#include "wayland_touch.hxx"

namespace {
class wayland_seat_wrapper
{
public:
	wayland_pointer_wrapper wayland_pointer;
	wayland_keyboard_wrapper wayland_keyboard;
	wayland_touch_wrapper wayland_touch;

	wayland_seat_wrapper(
		const wayland_registry_wrapper& wayland_registry, //
		const wayland_compositor_wrapper& wayland_compositor,
		const wayland_shm_wrapper& wayland_shm
	) :
		wayland_pointer(
			wayland_compositor, //
			wayland_shm
		),
		seat([&]() -> wl_seat* {
			if (!wayland_registry.seat_name.has_value()) {
				return nullptr;
			}

			void* seat = wl_registry_bind(
				wayland_registry.registry, //
				wayland_registry.seat_name.value().name,
				&wl_seat_interface,
				std::min(wayland_registry.seat_name.value().version, 1u)
			);
			utki::assert(seat, SL);
			return static_cast<wl_seat*>(seat);
		}())
	{
		// std::cout << "seat_wrapper constructor" << std::endl;
		if (!this->seat) {
			return;
		}
		wl_seat_add_listener(
			this->seat, //
			&listener,
			this
		);
	}

	wayland_seat_wrapper(const wayland_seat_wrapper&) = delete;
	wayland_seat_wrapper& operator=(const wayland_seat_wrapper&) = delete;

	wayland_seat_wrapper(wayland_seat_wrapper&&) = delete;
	wayland_seat_wrapper& operator=(wayland_seat_wrapper&&) = delete;

	~wayland_seat_wrapper()
	{
		if (!this->seat) {
			return;
		}

		if (wl_seat_get_version(this->seat) >= WL_SEAT_RELEASE_SINCE_VERSION) {
			wl_seat_release(this->seat);
		} else {
			wl_seat_destroy(this->seat);
		}
	}

private:
	wl_seat* const seat;

	static void wl_seat_capabilities(
		void* data, //
		wl_seat* wl_seat,
		uint32_t capabilities
	)
	{
		utki::log_debug([&](auto& o) {
			o << "seat capabilities: " << std::hex << "0x" << capabilities << std::endl;
		});

		auto& self = *static_cast<wayland_seat_wrapper*>(data);

		bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

		if (have_pointer) {
			utki::log_debug([&](auto& o) {
				o << "  pointer connected" << std::endl;
			});
			self.wayland_pointer.connect(self.seat);
		} else {
			utki::log_debug([&](auto& o) {
				o << "  pointer disconnected" << std::endl;
			});
			self.wayland_pointer.disconnect();
		}

		bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

		if (have_keyboard) {
			utki::log_debug([&](auto& o) {
				o << "  keyboard connected" << std::endl;
			});
			self.wayland_keyboard.connect(self.seat);
		} else {
			utki::log_debug([&](auto& o) {
				o << "  keyboard disconnected" << std::endl;
			});
			self.wayland_keyboard.disconnect();
		}

		bool have_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;

		if (have_touch) {
			utki::log_debug([&](auto& o) {
				o << "  touch connected" << std::endl;
			});
			self.wayland_touch.connect(self.seat);
		} else {
			utki::log_debug([&](auto& o) {
				o << "  touch disconnected" << std::endl;
			});
			self.wayland_touch.disconnect();
		}
	}

	constexpr static const wl_seat_listener listener = {
		.capabilities = &wl_seat_capabilities,
		.name =
			[](void* data, wl_seat* seat, const char* name) {
				utki::log_debug([&](auto& o) {
					o << "seat name: " << name << std::endl;
				});
			} //
	};
};
} // namespace
