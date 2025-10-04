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

#include <list>
#include <optional>
#include <string_view>

#include <utki/debug.hpp>
#include <utki/utility.hpp>
#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>

#include "wayland_display.hxx"
#include "wayland_output.hxx"

namespace {
struct wayland_registry_wrapper {
	wl_registry* registry;

	struct interface_name {
		uint32_t name;
		uint32_t version;
	};

	std::optional<interface_name> compositor_name;
	std::optional<interface_name> wm_base_name;
	std::optional<interface_name> shm_name;
	std::optional<interface_name> seat_name;

	std::list<wayland_output_wrapper> outputs;

	static void wl_registry_global(
		void* data, //
		wl_registry* registry,
		uint32_t id,
		const char* interface,
		uint32_t version
	);

	static void wl_registry_global_remove(
		void* data, //
		wl_registry* registry,
		uint32_t id
	);

	constexpr static const wl_registry_listener listener = {
		.global = &wl_registry_global,
		.global_remove = wl_registry_global_remove
	};

	wayland_registry_wrapper(wayland_display_wrapper& wayland_display);

	wayland_registry_wrapper(const wayland_registry_wrapper&) = delete;
	wayland_registry_wrapper& operator=(const wayland_registry_wrapper&) = delete;

	wayland_registry_wrapper(wayland_registry_wrapper&&) = delete;
	wayland_registry_wrapper& operator=(wayland_registry_wrapper&&) = delete;

	~wayland_registry_wrapper()
	{
		this->destroy();
	}

private:
	void destroy()
	{
		wl_registry_destroy(this->registry);
	}
};
} // namespace
