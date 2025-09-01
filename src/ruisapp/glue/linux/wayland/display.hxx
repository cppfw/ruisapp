#pragma once

#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#include "../../egl_utils.hxx"

#include "wayland_compositor.hxx"
#include "wayland_display.hxx"
#include "wayland_registry.hxx"
#include "wayland_seat.hxx"
#include "wayland_shm.hxx"
#include "xdg_wm_base.hxx"

namespace {
struct display_wrapper {
	wayland_display_wrapper wayland_display;
	wayland_registry_wrapper wayland_registry;
	wayland_compositor_wrapper wayland_compositor;
	wayland_shm_wrapper wayland_shm;
	xdg_wm_base_wrapper xdg_wm_base;
	wayland_seat_wrapper wayland_seat;

	egl_display_wrapper egl_display;

	display_wrapper() :
		wayland_registry(this->wayland_display),
		wayland_compositor(this->wayland_registry),
		wayland_shm(this->wayland_registry),
		xdg_wm_base(this->wayland_registry),
		wayland_seat(
			this->wayland_registry, //
			this->wayland_compositor,
			this->wayland_shm
		),
		egl_display(this->wayland_display.display)
	{}

	display_wrapper(const display_wrapper&) = delete;
	display_wrapper& operator=(const display_wrapper&) = delete;

	display_wrapper(display_wrapper&&) = delete;
	display_wrapper& operator=(display_wrapper&&) = delete;

	~display_wrapper() = default;
};
} // namespace
