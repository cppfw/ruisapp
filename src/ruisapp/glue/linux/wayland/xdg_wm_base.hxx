#pragma once

#include "wayland_registry.hxx"

namespace {
struct xdg_wm_base_wrapper {
	xdg_wm_base* const wm_base;

	xdg_wm_base_wrapper(const wayland_registry_wrapper& wayland_registry) :
		wm_base([&]() {
			utki::assert(wayland_registry.wm_base_name.has_value(), SL);
			void* wb = wl_registry_bind(
				wayland_registry.registry,
				wayland_registry.wm_base_name.value().name,
				&xdg_wm_base_interface,
				std::min(wayland_registry.wm_base_name.value().version, 2u)
			);
			utki::assert(wb, SL);
			return static_cast<xdg_wm_base*>(wb);
		}())
	{
		xdg_wm_base_add_listener(
			this->wm_base, //
			&listener,
			this
		);
	}

	xdg_wm_base_wrapper(const xdg_wm_base_wrapper&) = delete;
	xdg_wm_base_wrapper& operator=(const xdg_wm_base_wrapper&) = delete;

	xdg_wm_base_wrapper(xdg_wm_base_wrapper&&) = delete;
	xdg_wm_base_wrapper& operator=(xdg_wm_base_wrapper&&) = delete;

	~xdg_wm_base_wrapper()
	{
		xdg_wm_base_destroy(this->wm_base);
	}

private:
	constexpr static const xdg_wm_base_listener listener = {
		.ping =
			[](void* data, //
			   xdg_wm_base* wm_base,
			   uint32_t serial //
			) {
				xdg_wm_base_pong(
					wm_base, //
					serial
				);
			} //
	};
};
} // namespace
