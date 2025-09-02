#pragma once

#include <map>
#include <stdexcept>

#include <ruis/config.hpp>
#include <utki/debug.hpp>
#include <wayland-client-protocol.h>

namespace {
class wayland_touch_wrapper
{
	wl_touch* touch = nullptr;

	unsigned num_connected = 0;

	struct touch_point {
		unsigned ruis_id;
		ruis::vec2 pos;
		wl_surface* surface;
	};

	std::map<int32_t, touch_point> touch_points;

	void release()
	{
		utki::assert(this->touch, SL);
		if (wl_touch_get_version(this->touch) >= WL_TOUCH_RELEASE_SINCE_VERSION) {
			wl_touch_release(this->touch);
		} else {
			wl_touch_destroy(this->touch);
		}
	}

	// TODO: move implementation here?
	static void wl_touch_down(
		void* data, //
		wl_touch* touch,
		uint32_t serial,
		uint32_t time,
		wl_surface* surface,
		int32_t id,
		wl_fixed_t x,
		wl_fixed_t y
	);

	// TODO: move implementation here?
	static void wl_touch_up(
		void* data, //
		wl_touch* touch,
		uint32_t serial,
		uint32_t time,
		int32_t id
	);

	// TODO: move implementation here?
	static void wl_touch_motion(
		void* data, //
		wl_touch* touch,
		uint32_t time,
		int32_t id,
		wl_fixed_t x,
		wl_fixed_t y
	);

	static void wl_touch_frame(
		void* data, //
		wl_touch* touch
	)
	{
		utki::log_debug([](auto& o) {
			o << "wayland: touch frame event" << std::endl;
		});
	}

	static void wl_touch_cancel(
		void* data, //
		wl_touch* touch
	);

	static void wl_touch_shape(
		void* data, //
		wl_touch* touch,
		int32_t id,
		wl_fixed_t major,
		wl_fixed_t minor
	)
	{
		utki::log_debug([](auto& o) {
			o << "wayland: touch shape event" << std::endl;
		});
	}

	static void wl_touch_orientation(
		void* data, //
		wl_touch* touch,
		int32_t id,
		wl_fixed_t orientation
	)
	{
		utki::log_debug([](auto& o) {
			o << "wayland: touch orientation event" << std::endl;
		});
	}

	constexpr static const wl_touch_listener listener = {
		.down = &wl_touch_down,
		.up = &wl_touch_up,
		.motion = &wl_touch_motion,
		.frame = &wl_touch_frame,
		.cancel = &wl_touch_cancel,
		.shape = &wl_touch_shape,
		.orientation = &wl_touch_orientation
	};

public:
	wayland_touch_wrapper() = default;

	wayland_touch_wrapper(const wayland_touch_wrapper&) = delete;
	wayland_touch_wrapper& operator=(const wayland_touch_wrapper&) = delete;

	wayland_touch_wrapper(wayland_touch_wrapper&&) = delete;
	wayland_touch_wrapper& operator=(wayland_touch_wrapper&&) = delete;

	~wayland_touch_wrapper()
	{
		if (this->touch) {
			this->release();
		}
	}

	void connect(wl_seat* seat)
	{
		++this->num_connected;

		if (this->num_connected > 1) {
			// already connected
			utki::assert(this->touch, SL);
			return;
		}

		this->touch = wl_seat_get_touch(seat);
		if (!this->touch) {
			this->num_connected = 0;
			throw std::runtime_error("could not get wayland touch interface");
		}

		if (wl_touch_add_listener(
				this->touch, //
				&listener,
				this
			) != 0)
		{
			this->release();
			this->num_connected = 0;
			throw std::runtime_error("could not add listener to wayland touch interface");
		}
	}

	void disconnect() noexcept
	{
		if (this->num_connected == 0) {
			// no pointers connected
			utki::assert(!this->touch, SL);
			return;
		}

		utki::assert(this->touch, SL);

		--this->num_connected;

		if (this->num_connected == 0) {
			this->release();
			this->touch = nullptr;
		}
	}
};
} // namespace
