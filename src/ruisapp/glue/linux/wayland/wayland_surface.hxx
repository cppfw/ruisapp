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

	scale_and_dpi find_scale_and_dpi(const std::map<uint32_t, wayland_output_wrapper>& wayland_outputs)
	{
		utki::log_debug([](auto& o) {
			o << "looking for max scale" << std::endl;
		});

		// if surface did not enter any outputs yet, then just take scale of first available output
		if (this->wayland_outputs.empty()) {
			utki::log_debug([](auto& o) {
				o << "  surface has not entered any outputs yet" << std::endl;
			});
			return {};
		}

		scale_and_dpi max_sd;

		// go through outputs which the surface has entered
		for (auto wlo : this->wayland_outputs) {
			auto id = get_output_id(wlo);

			utki::log_debug([&](auto& o) {
				o << "  check output id = " << id << std::endl;
			});

			auto i = wayland_outputs.find(id);
			if (i == wayland_outputs.end()) {
				utki::log_debug([&](auto& o) {
					o << "WARNING: wayland surface entered output with id = " << id
					  << ", but no output with such id is reported" << std::endl;
				});
				continue;
			}

			auto& output = i->second;

			utki::log_debug([&](auto& o) {
				o << "  output found, scale = " << output.scale << std::endl;
			});

			max_sd.scale = std::max(output.scale, max_sd.scale);
			max_sd.dpi = output.get_dpi();
		}

		return max_sd;
	}

private:
	static uint32_t get_output_id(wl_output* output)
	{
		utki::assert(output, SL);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return wl_proxy_get_id(reinterpret_cast<wl_proxy*>(output));
	}

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
