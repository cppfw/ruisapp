#pragma once

#include "wayland_registry.hxx"

namespace {
struct wayland_compositor_wrapper {
	wl_compositor* const compositor;

	wayland_compositor_wrapper(const wayland_registry_wrapper& wayland_registry) :
		compositor([&]() {
			utki::assert(wayland_registry.compositor_id.has_value(), SL);
			void* comp = wl_registry_bind(
				wayland_registry.registry,
				wayland_registry.compositor_id.value().id,
				&wl_compositor_interface,
				std::min(wayland_registry.compositor_id.value().version, 4u)
			);
			utki::assert(comp, SL);
			return static_cast<wl_compositor*>(comp);
		}())
	{}

	wayland_compositor_wrapper(const wayland_compositor_wrapper&) = delete;
	wayland_compositor_wrapper& operator=(const wayland_compositor_wrapper&) = delete;

	wayland_compositor_wrapper(wayland_compositor_wrapper&&) = delete;
	wayland_compositor_wrapper& operator=(wayland_compositor_wrapper&&) = delete;

	~wayland_compositor_wrapper()
	{
		wl_compositor_destroy(this->compositor);
	}
};
} // namespace
