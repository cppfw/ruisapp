#pragma once

#include <r4/rectangle.hpp>

#include "wayland_compositor.hxx"

namespace {
struct wayland_region_wrapper {
	wl_region* region;

	wayland_region_wrapper(const wayland_compositor_wrapper& wayland_compositor) :
		region(wl_compositor_create_region(wayland_compositor.compositor))
	{
		if (!this->region) {
			throw std::runtime_error("could not create wayland region");
		}
	}

	wayland_region_wrapper(const wayland_region_wrapper&) = delete;
	wayland_region_wrapper& operator=(const wayland_region_wrapper&) = delete;

	wayland_region_wrapper(wayland_region_wrapper&&) = delete;
	wayland_region_wrapper& operator=(wayland_region_wrapper&&) = delete;

	~wayland_region_wrapper()
	{
		wl_region_destroy(this->region);
	}

	void add(const r4::rectangle<int32_t>& rect)
	{
		wl_region_add(
			this->region, //
			rect.p.x(),
			rect.p.y(),
			rect.d.x(),
			rect.d.y()
		);
	}
};
} // namespace
