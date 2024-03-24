/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2024  Ivan Gagis <igagis@gmail.com>

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

#include <utki/destructable.hpp>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "../../../out/xdg-shell-client-protocol.h"

#include "../../application.hpp"

using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
struct window_wrapper : public utki::destructable {
	struct display_wrapper {
		wl_display* disp;

		wl_compositor* compositor = nullptr;
		xdg_wm_base* xdg_wm_base = nullptr;

		static void xdg_wm_base_ping(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial)
		{
			xdg_wm_base_pong(xdg_wm_base, serial);
		}

		static const xdg_wm_base_listener xdg_wm_base_listener = {
			.ping = &xdg_wm_base_ping //
		};

		static void global_registry_handler( //
			void* data,
			wl_registry* registry,
			uint32_t id,
			const char* interface,
			uint32_t version
		)
		{
			ASSERT(data)
			auto& self = *static_cast<display_wrapper*>(data);

			LOG([](auto& o) {
				o << "got a registry event for: " << interface << ", id = " << id << std::endl;
			});
			if (std::string_view(interface) == "wl_compositor"sv) {
				self.compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
			} else if (std::string_view(interface) == xdg_wm_base_interface.name) {
				self.xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
				xdg_wm_base_add_listener(self.xdg_wm_base, &xdg_wm_base_listener, NULL);
			}
		}

		static void global_registry_remover( //
			void* data,
			struct wl_registry* registry,
			uint32_t id
		)
		{
			LOG([](auto& o) {
				o << "got a registry losing event, id = " << id << std::endl;
			});
		}

		static const wl_registry_listener listener = {
			&global_registry_handler, //
			&global_registry_remover
		};

		display_wrapper() :
			disp(wl_display_connect(NULL))
		{
			if (!this->disp) {
				throw std::runtime_error("could not connect to wayland display");
			}

			wl_registry* registry = wl_display_get_registry(this->disp);
			ASSERT(registry)
			wl_registry_add_listener(registry, &listener, this);

			// this will call the attached listener global_registry_handler
			wl_display_dispatch(this->disp);
			wl_display_roundtrip(this->disp);
		}

		~display_wrapper()
		{
			wl_display_disconnect(this->disp);
		}
	} display;

	window_wrapper(const window_params& wp) {}
};
} // namespace

int main(int argc, const char** argv)
{
	return 0;
}
