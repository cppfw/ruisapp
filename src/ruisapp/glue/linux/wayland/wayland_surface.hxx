#pragma once

#include <set>

#include "wayland_compositor.hxx"
#include "wayland_region.hxx"

namespace {
struct wayland_surface_wrapper {
	wl_surface* const surface;

	wayland_surface_wrapper(const wayland_compositor_wrapper& wayland_compositor) :
		surface(wl_compositor_create_surface(wayland_compositor.compositor))
	{
		if (!this->surface) {
			throw std::runtime_error("could not create wayland surface");
		}

		wl_surface_add_listener(
			this->surface, //
			&listener,
			this
		);
	}

	wayland_surface_wrapper(const wayland_surface_wrapper&) = delete;
	wayland_surface_wrapper& operator=(const wayland_surface_wrapper&) = delete;

	wayland_surface_wrapper(wayland_surface_wrapper&&) = delete;
	wayland_surface_wrapper& operator=(wayland_surface_wrapper&&) = delete;

	~wayland_surface_wrapper()
	{
		wl_surface_destroy(this->surface);
	}

	void commit()
	{
		wl_surface_commit(this->surface);
	}

	void set_buffer_scale(uint32_t scale)
	{
		if (wl_surface_get_version(this->surface) >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION) {
			wl_surface_set_buffer_scale(
				this->surface, //
				int32_t(scale)
			);
		}
	}

	void set_opaque_region(const wayland_region_wrapper& wayland_region)
	{
		wl_surface_set_opaque_region(
			this->surface, //
			wayland_region.region
		);
	}

	struct scale_and_dpi {
		uint32_t scale = 1;

		constexpr static const float default_dpi = 96;
		float dpi = default_dpi;
	};

	scale_and_dpi find_scale_and_dpi(const std::list<wayland_output_wrapper>& wayland_outputs);

private:
	// wayland outputs this surface is on
	std::set<wl_output*> wayland_outputs;

	static void wl_surface_enter(
		void* data, //
		wl_surface* surface,
		wl_output* output
	);

	static void wl_surface_leave(
		void* data, //
		wl_surface* surface,
		wl_output* output
	);

	constexpr static const wl_surface_listener listener = {
		.enter = &wl_surface_enter, //
		.leave = &wl_surface_leave
	};
};
} // namespace
