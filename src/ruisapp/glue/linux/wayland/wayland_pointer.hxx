#pragma once

#include <ruis/config.hpp>
#include <ruis/util/events.hpp>

#include "wayland_cursor_theme.hxx"
#include "wayland_surface.hxx"

namespace {
struct wayland_pointer_wrapper {
	wayland_cursor_theme_wrapper wayland_cursor_theme;

	wayland_surface_wrapper cursor_wayland_surface;

	ruis::vector2 cur_pointer_pos{0, 0};

	void connect(wl_seat* seat)
	{
		++this->num_connected;

		if (this->num_connected > 1) {
			// already connected
			utki::assert(this->pointer, SL);
			return;
		}

		this->pointer = wl_seat_get_pointer(seat);
		if (!this->pointer) {
			this->num_connected = 0;
			throw std::runtime_error("could not get wayland pointer interface");
		}

		if (wl_pointer_add_listener(this->pointer, &listener, this) != 0) {
			this->release();
			this->num_connected = 0;
			throw std::runtime_error("could not add listener to wayland pointer interface");
		}
	}

	void disconnect() noexcept
	{
		if (this->num_connected == 0) {
			// no pointers connected
			utki::assert(!this->pointer, SL);
			return;
		}

		utki::assert(this->pointer, SL);

		--this->num_connected;

		if (this->num_connected == 0) {
			this->release();
			this->pointer = nullptr;
		}
	}

	void set_cursor()
	{
		if (this->cursor_visible) {
			this->apply_cursor(this->current_cursor);
		} else {
			wl_pointer_set_cursor(
				this->pointer, //
				this->last_enter_serial,
				nullptr,
				0,
				0
			);
		}
	}

	void set_cursor(ruis::mouse_cursor c)
	{
		this->current_cursor = this->wayland_cursor_theme.get(c);

		this->set_cursor();
	}

	void set_cursor_visible(bool visible)
	{
		this->cursor_visible = visible;

		if (!this->pointer) {
			return;
		}

		if (visible) {
			this->apply_cursor(this->current_cursor);
		} else {
			wl_pointer_set_cursor(
				this->pointer, //
				this->last_enter_serial,
				nullptr,
				0,
				0
			);
		}
	}

	wayland_pointer_wrapper(
		const wayland_compositor_wrapper& wayland_compositor, //
		const wayland_shm_wrapper& wayland_shm
	) :
		wayland_cursor_theme(wayland_shm),
		cursor_wayland_surface(wayland_compositor),
		current_cursor(this->wayland_cursor_theme.get(ruis::mouse_cursor::arrow))
	{}

	wayland_pointer_wrapper(const wayland_pointer_wrapper&) = delete;
	wayland_pointer_wrapper& operator=(const wayland_pointer_wrapper&) = delete;

	wayland_pointer_wrapper(wayland_pointer_wrapper&&) = delete;
	wayland_pointer_wrapper& operator=(wayland_pointer_wrapper&&) = delete;

	~wayland_pointer_wrapper()
	{
		if (this->pointer) {
			this->release();
		}
	}

private:
	static ruis::mouse_button button_number_to_enum(uint32_t number)
	{
		// from wayland's comments:
		// The number is a button code as defined in the Linux kernel's
		// linux/input-event-codes.h header file, e.g. BTN_LEFT.

		switch (number) {
			case 0x110: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::left;
			default:
			case 0x112: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::middle;
			case 0x111: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::right;
			case 0x113: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::side;
			case 0x114: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::extra;
			case 0x115: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::forward;
			case 0x116: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::back;
			case 0x117: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
				return ruis::mouse_button::task;
		}
	}

	void release()
	{
		if (wl_pointer_get_version(this->pointer) >= WL_POINTER_RELEASE_SINCE_VERSION) {
			wl_pointer_release(this->pointer);
		} else {
			wl_pointer_destroy(this->pointer);
		}
	}

	static void wl_pointer_enter(
		void* data, //
		wl_pointer* pointer,
		uint32_t serial,
		wl_surface* surface,
		wl_fixed_t x,
		wl_fixed_t y
	)
	{
		// std::cout << "mouse enter: x,y = " << std::dec << x << ", " << y << std::endl;
		auto& self = *static_cast<wayland_pointer_wrapper*>(data);
		self.last_enter_serial = serial;

		self.set_cursor();

		handle_mouse_hover(
			ruisapp::inst(), //
			true,
			0
		);
		self.cur_pointer_pos = ruis::vector2(
			wl_fixed_to_int(x), //
			wl_fixed_to_int(y)
		);
		handle_mouse_move(
			ruisapp::inst(), //
			self.cur_pointer_pos,
			0
		);
	}

	// TODO: move implementation here?
	static void wl_pointer_motion(
		void* data, //
		wl_pointer* pointer,
		uint32_t time,
		wl_fixed_t x,
		wl_fixed_t y
	);

	static void wl_pointer_button(
		void* data, //
		wl_pointer* pointer,
		uint32_t serial,
		uint32_t time,
		uint32_t button,
		uint32_t state
	)
	{
		// std::cout << "mouse button: " << std::hex << "0x" << button << ", state = " << "0x" << state <<
		// std::endl;
		auto& self = *static_cast<wayland_pointer_wrapper*>(data);
		handle_mouse_button(
			ruisapp::inst(), //
			state == WL_POINTER_BUTTON_STATE_PRESSED,
			self.cur_pointer_pos,
			button_number_to_enum(button),
			0
		);
	}

	static void wl_pointer_axis(
		void* data, //
		wl_pointer* pointer,
		uint32_t time,
		uint32_t axis,
		wl_fixed_t value
	)
	{
		auto& self = *static_cast<wayland_pointer_wrapper*>(data);

		// we get +-10 for each mouse wheel step
		auto val = wl_fixed_to_int(value);

		// std::cout << "mouse axis: " << std::dec << axis << ", val = " << val << std::endl;

		for (unsigned i = 0; i != 2; ++i) {
			handle_mouse_button(
				ruisapp::inst(), //
				i == 0, // pressed/released
				self.cur_pointer_pos,
				[axis, val]() {
					if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
						if (val >= 0) {
							return ruis::mouse_button::wheel_down;
						} else {
							return ruis::mouse_button::wheel_up;
						}
					} else {
						if (val >= 0) {
							return ruis::mouse_button::wheel_right;
						} else {
							return ruis::mouse_button::wheel_left;
						}
					}
				}(),
				0 // pointer id
			);
		}
	};

	constexpr static const wl_pointer_listener listener = {
		.enter = &wl_pointer_enter,
		.leave =
			[](void* data, //
			   wl_pointer* pointer,
			   uint32_t serial,
			   wl_surface* surface) {
				// std::cout << "mouse leave" << std::endl;
				handle_mouse_hover(ruisapp::inst(), false, 0);
			},
		.motion = &wl_pointer_motion,
		.button = &wl_pointer_button,
		.axis = &wl_pointer_axis,
		.frame =
			[](void* data, //
			   wl_pointer* pointer) {
				utki::log_debug([](auto& o) {
					o << "pointer frame" << std::endl;
				});
			},
		.axis_source =
			[](void* data, //
			   wl_pointer* pointer,
			   uint32_t source) {
				utki::log_debug([&](auto& o) {
					o << "axis source: " << std::dec << source << std::endl;
				});
			},
		.axis_stop =
			[](void* data, //
			   wl_pointer* pointer,
			   uint32_t time,
			   uint32_t axis) {
				utki::log_debug([&](auto& o) {
					o << "axis stop: axis = " << std::dec << axis << std::endl;
				});
			},
		.axis_discrete =
			[](void* data, //
			   wl_pointer* pointer,
			   uint32_t axis,
			   int32_t discrete) {
				utki::log_debug([&](auto& o) {
					o << "axis discrete: axis = " << std::dec << axis << ", discrete = " << discrete << std::endl;
				});
			}
	};

	unsigned num_connected = 0;

	wl_pointer* pointer = nullptr;

	bool cursor_visible = true;

	// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
	wl_cursor* current_cursor;

	uint32_t last_enter_serial = 0;

	void apply_cursor(wl_cursor* cursor)
	{
		if (!this->pointer) {
			// no pointer connected
			return;
		}

		utki::scope_exit scope_exit_empty_cursor([this]() {
			wl_pointer_set_cursor(
				this->pointer, //
				this->last_enter_serial,
				nullptr,
				0,
				0
			);
		});

		if (!cursor || cursor->image_count < 1) {
			return;
		}

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic, "using C api")
		wl_cursor_image* image = cursor->images[0];
		if (!image) {
			return;
		}

		wl_buffer* buffer = wl_cursor_image_get_buffer(image);
		if (!buffer) {
			return;
		}

		wl_pointer_set_cursor(
			this->pointer, //
			this->last_enter_serial,
			this->cursor_wayland_surface.surface,
			int32_t(image->hotspot_x),
			int32_t(image->hotspot_y)
		);

		wl_surface_attach(
			this->cursor_wayland_surface.surface, //
			buffer,
			0,
			0
		);

		wl_surface_damage(
			this->cursor_wayland_surface.surface, //
			0,
			0,
			int32_t(image->width),
			int32_t(image->height)
		);

		this->cursor_wayland_surface.commit();

		scope_exit_empty_cursor.release();
	}
};
} // namespace
