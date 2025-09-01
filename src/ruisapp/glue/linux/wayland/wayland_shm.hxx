#pragma once

#include "wayland_registry.hxx"

namespace {
struct wayland_shm_wrapper {
	wl_shm* const shm;

	wayland_shm_wrapper(const wayland_registry_wrapper& wayland_registry) :
		shm([&]() {
			utki::assert(wayland_registry.shm_id.has_value(), SL);
			void* s = wl_registry_bind(
				wayland_registry.registry,
				wayland_registry.shm_id.value().id,
				&wl_shm_interface,
				std::min(wayland_registry.shm_id.value().version, 1u)
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
