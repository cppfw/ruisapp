#include "wayland_pointer.hxx"

#include "application.hxx"

void wayland_pointer_wrapper::wl_pointer_enter(
	void* data, //
	wl_pointer* pointer,
	uint32_t serial,
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

	self.last_enter_serial = serial;

	self.set_cursor();

	auto& glue = get_glue();

	auto window = glue.get_window(surface);
	if (!window) {
		return;
	}
	auto& win = *window;

	win.gui.send_mouse_hover(
		true, //
		0
	);
	self.cur_pointer_pos = ruis::vector2(
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
		state == WL_POINTER_BUTTON_STATE_PRESSED, //
		self.cur_pointer_pos,
		button_number_to_enum(button),
		0
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

	ruis::vector2 pos( //
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
