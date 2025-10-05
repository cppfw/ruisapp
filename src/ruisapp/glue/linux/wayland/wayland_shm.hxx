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

#include "wayland_registry.hxx"

namespace {
struct wayland_shm_wrapper {
	wl_shm* const shm;

	wayland_shm_wrapper(const wayland_registry_wrapper& wayland_registry) :
		shm([&]() {
			utki::assert(wayland_registry.shm_name.has_value(), SL);
			void* s = wl_registry_bind(
				wayland_registry.registry,
				wayland_registry.shm_name.value().name,
				&wl_shm_interface,
				std::min(wayland_registry.shm_name.value().version, 1u)
			);
			utki::assert(s, SL);
			return static_cast<wl_shm*>(s);
		}())
	{}

	wayland_shm_wrapper(const wayland_shm_wrapper&) = delete;
	wayland_shm_wrapper& operator=(const wayland_shm_wrapper&) = delete;

	wayland_shm_wrapper(wayland_shm_wrapper&&) = delete;
	wayland_shm_wrapper& operator=(wayland_shm_wrapper&&) = delete;

	~wayland_shm_wrapper()
	{
		wl_shm_destroy(this->shm);
	}
};
} // namespace
