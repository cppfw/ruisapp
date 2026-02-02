/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2025  Ivan Gagis <igagis@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* ================ LICENSE END ================ */

#include "wayland_pointer.hxx"

#include "application.hxx"

namespace {
ruis::mouse_button button_number_to_enum(uint32_t number)
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
} // namespace

void wayland_pointer_wrapper::connect(wl_seat* seat)
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

	if (wl_pointer_add_listener(
			this->pointer, //
			&listener,
			this
		) != 0)
	{
		this->release();
		this->num_connected = 0;
		throw std::runtime_error("could not add listener to wayland pointer interface");
	}
}

void wayland_pointer_wrapper::disconnect() noexcept
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

void wayland_pointer_wrapper::wl_pointer_enter(
	void* data, //
	wl_pointer* pointer,
	uint32_t event_serial_number,
	wl_surface* surface,
	wl_fixed_t x,
	wl_fixed_t y
)
{
	// std::cout << "mouse enter: x,y = " << std::dec << x << ", " << y << std::endl;

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_pointer_wrapper*>(data);

	utki::assert(self.cur_surface == nullptr, SL);
	self.cur_surface = surface;

	self.last_enter_event_serial_number = event_serial_number;

	auto& glue = get_glue();

	auto window = glue.get_window(surface);
	if (!window) {
		return;
	}
	auto& win = *window;

	auto& natwin = win.ruis_native_window.get();

	natwin.update_mouse_cursor();

	win.gui.send_mouse_hover(
		true, //
		0
	);
	self.cur_pointer_pos = ruis::vec2(
		wl_fixed_to_int(x), //
		wl_fixed_to_int(y)
	);
	win.gui.send_mouse_move(
		self.cur_pointer_pos, //
		0
	);
}

void wayland_pointer_wrapper::wl_pointer_leave(
	void* data, //
	wl_pointer* pointer,
	uint32_t serial,
	wl_surface* surface
)
{
	// std::cout << "mouse leave" << std::endl;

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_pointer_wrapper*>(data);

	if (!surface) {
		// This should not happen according to Wayland protocol, but it happens on practice.
		// For example when Wayland window which was hovered by pointer is destroyed.
		// Just reset currently hovered surface.
		self.cur_surface = nullptr;
		return;
	}

	utki::assert(self.cur_surface == surface, SL);

	self.cur_surface = nullptr;

	auto& glue = get_glue();

	auto window = glue.get_window(surface);
	if (!window) {
		return;
	}

	window->gui.send_mouse_hover(
		false, //
		0
	);
}

void wayland_pointer_wrapper::wl_pointer_button(
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
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_pointer_wrapper*>(data);

	if (!self.cur_surface) {
		utki::log_debug([](auto& o) {
			o << "wayland_pointer_wrapper::wl_pointer_button(): no surface, ignore" << std::endl;
		});
		return;
	}

	auto& glue = get_glue();
	auto window = glue.get_window(self.cur_surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_pointer_wrapper::wl_pointer_button(): no window for current surface, ignore" << std::endl;
		});
		return;
	}

	window->gui.send_mouse_button(
		state == WL_POINTER_BUTTON_STATE_PRESSED ? ruis::button_action::press : ruis::button_action::release, //
		self.cur_pointer_pos,
		button_number_to_enum(button),
		0 // pointer_id
	);
}

void wayland_pointer_wrapper::wl_pointer_axis(
	void* data, //
	wl_pointer* pointer,
	uint32_t time,
	uint32_t axis,
	wl_fixed_t value
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_pointer_wrapper*>(data);

	auto& glue = get_glue();

	// TODO: is it a valid situation that there is no current surface?
	utki::assert(self.cur_surface, SL);

	auto window = glue.get_window(self.cur_surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_pointer_wrapper::wl_pointer_axis(): no window for current surface, ignore" << std::endl;
		});
		return;
	}
	auto& win = *window;

	// we get +-10 for each mouse wheel step
	auto val = wl_fixed_to_int(value);

	// std::cout << "mouse axis: " << std::dec << axis << ", val = " << val << std::endl;

	for (unsigned i = 0; i != 2; ++i) {
		win.gui.send_mouse_button(
			i == 0 ? ruis::button_action::press : ruis::button_action::release, //
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
}

void wayland_pointer_wrapper::wl_pointer_motion(
	void* data, //
	wl_pointer* pointer,
	uint32_t time,
	wl_fixed_t x,
	wl_fixed_t y
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_pointer_wrapper*>(data);

	auto& glue = get_glue();
	auto window = glue.get_window(self.cur_surface);
	if (!window) {
		utki::logcat_debug("wayland_pointer_wrapper::wl_pointer_motion(): no current surface, ignore event", '\n');
		return;
	}

	auto& win = *window;

	ruis::vec2 pos( //
		ruis::real(wl_fixed_to_double(x)),
		ruis::real(wl_fixed_to_double(y))
	);
	pos *= win.ruis_native_window.get().get_scale();
	self.cur_pointer_pos = round(pos);

	// std::cout << "mouse move: x,y = " << std::dec << self.cur_pointer_pos << std::endl;
	win.gui.send_mouse_move(
		self.cur_pointer_pos, //
		0
	);
}

void wayland_pointer_wrapper::set_cursor(ruis::mouse_cursor c)
{
	this->apply_cursor(this->wayland_cursor_theme.get(c));
}

void wayland_pointer_wrapper::apply_cursor(wl_cursor* cursor)
{
	if (!this->pointer) {
		// no pointer connected
		return;
	}

	utki::scope_exit scope_exit_empty_cursor([this]() {
		wl_pointer_set_cursor(
			this->pointer, //
			this->last_enter_event_serial_number,
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
		this->last_enter_event_serial_number,
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
