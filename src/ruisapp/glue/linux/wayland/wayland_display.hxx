#pragma once

#include <stdexcept>

#include <wayland-client-core.h>

namespace {
struct wayland_display_wrapper {
	wl_display* display;

	wayland_display_wrapper() :
		display(wl_display_connect(nullptr))
	{
		if (!this->display) {
			throw std::runtime_error("wl_display_connect(): failed");
		}
	}

	wayland_display_wrapper(const wayland_display_wrapper&) = delete;
	wayland_display_wrapper& operator=(const wayland_display_wrapper&) = delete;
	wayland_display_wrapper(wayland_display_wrapper&&) = delete;
	wayland_display_wrapper& operator=(wayland_display_wrapper&&) = delete;

	~wayland_display_wrapper()
	{
		wl_display_disconnect(this->display);
	}
};
} // namespace
