#pragma once

#include <r4/vector.hpp>
#include <utki/debug.hpp>
#include <wayland-client-protocol.h>

namespace {
class wayland_output_wrapper
{
	wl_output* output;

	// TODO: move implementation here
	static void wl_output_geometry(
		void* data,
		struct wl_output* wl_output,
		int32_t x,
		int32_t y,
		int32_t physical_width,
		int32_t physical_height,
		int32_t subpixel,
		const char* make,
		const char* model,
		int32_t transform
	);

	// TODO: move implementation here
	static void wl_output_mode(
		void* data,
		struct wl_output* wl_output,
		uint32_t flags,
		int32_t width,
		int32_t height,
		int32_t refresh
	);

	// NOTE: done event only comes from wl_output_interface version 2 or above.
	static void wl_output_done(
		void* data, //
		struct wl_output* wl_output
	)
	{
		utki::assert(data, SL);

		auto& self = *static_cast<wayland_output_wrapper*>(data);

		utki::log_debug([&](auto& o) {
			o << "output(" << self.id << ") done" << std::endl;
		});
	}

	// TODO: move implementation here?
	static void wl_output_scale(
		void* data, //
		struct wl_output* wl_output,
		int32_t factor
	);

	static void wl_output_name(
		void* data, //
		struct wl_output* wl_output,
		const char* name
	)
	{
		utki::assert(data, SL);
		auto& self = *static_cast<wayland_output_wrapper*>(data);

		self.name = name;

		utki::log_debug([&](auto& o) {
			o << "output(" << self.id << ") name = " << self.name << std::endl;
		});
	}

	static void wl_output_description(
		void* data, //
		struct wl_output* wl_output,
		const char* description
	)
	{
		utki::assert(data, SL);
		auto& self = *static_cast<wayland_output_wrapper*>(data);

		self.description = description;

		utki::log_debug([&](auto& o) {
			o << "output(" << self.id << ") description = " << self.description << std::endl;
		});
	}

	constexpr static const wl_output_listener listener = {
		.geometry = &wl_output_geometry,
		.mode = &wl_output_mode,
		.done = &wl_output_done,
		.scale = &wl_output_scale,

		// TODO: wayland version included in debian bullseye does not support these fields,
		//       uncomment them when debian bullseye support can be dropped
		// .name = &wl_output_name,
		// .description = &wl_output_description
	};

public:
	const uint32_t id;

	r4::vector2<uint32_t> position = {0, 0};
	r4::vector2<uint32_t> resolution = {0, 0};
	r4::vector2<uint32_t> physical_size_mm = {0, 0};
	std::string name;
	std::string description;
	uint32_t scale = 1;

	float get_dpi() const noexcept
	{
		auto dpi = (this->resolution.to<float>() * utki::mm_per_inch).comp_div(this->physical_size_mm.to<float>());
		return (dpi.x() + dpi.y()) / 2;
	}

	wayland_output_wrapper(
		wl_registry& registry, //
		uint32_t id,
		uint32_t interface_version
	) :
		output([&]() {
			void* output = wl_registry_bind(
				&registry, //
				id,
				&wl_output_interface,
				std::min(interface_version, 2u)
			);
			utki::assert(output, SL);
			return static_cast<wl_output*>(output);
		}()),
		id(id)
	{
		wl_output_add_listener(this->output, &listener, this);
	}

	wayland_output_wrapper(const wayland_output_wrapper&) = delete;
	wayland_output_wrapper& operator=(const wayland_output_wrapper&) = delete;

	wayland_output_wrapper(wayland_output_wrapper&&) = delete;
	wayland_output_wrapper& operator=(wayland_output_wrapper&&) = delete;

	~wayland_output_wrapper()
	{
		if (wl_output_get_version(this->output) >= WL_OUTPUT_RELEASE_SINCE_VERSION) {
			wl_output_release(this->output);
		} else {
			wl_output_destroy(this->output);
		}
	}
};
} // namespace
