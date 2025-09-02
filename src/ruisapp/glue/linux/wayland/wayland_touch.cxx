#include "wayland_touch.hxx"

#include "application.hxx"

void wayland_touch_wrapper::wl_touch_down(
	void* data, //
	wl_touch* touch,
	uint32_t serial,
	uint32_t time,
	wl_surface* surface,
	int32_t id,
	wl_fixed_t x,
	wl_fixed_t y
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch down event" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_touch_wrapper*>(data);
	utki::assert(self.touch == touch, SL);

	auto& glue = get_glue();
	auto window = glue.get_window(surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_touch_wrapper::wl_touch_down(): non-window surface touched, ignore" << std::endl;
		});
		return;
	}
	auto& win = *window;

	utki::assert(!utki::contains(self.touch_points, id), SL);

	ruis::vector2 pos( //
		ruis::real(wl_fixed_to_double(x)),
		ruis::real(wl_fixed_to_double(y))
	);
	pos = round(pos * win.ruis_native_window.get().get_scale());

	auto insert_result = self.touch_points.insert(std::make_pair(
		id,
		touch_point{
			.ruis_id = unsigned(id) + 1, // id = 0 reserved for mouse
			.pos = pos,
			.surface = surface
		}
	));

	// pair successfully inserted
	utki::assert(insert_result.second, SL);

	const touch_point& tp = insert_result.first->second;

	win.gui.send_mouse_button(
		true, // is_down
		tp.pos,
		ruis::mouse_button::left,
		tp.ruis_id
	);
}

void wayland_touch_wrapper::wl_touch_up(
	void* data, //
	wl_touch* touch,
	uint32_t serial,
	uint32_t time,
	int32_t id
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch up event" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_touch_wrapper*>(data);
	utki::assert(self.touch == touch, SL);

	auto i = self.touch_points.find(id);
	utki::assert(i != self.touch_points.end(), SL);

	const touch_point& tp = i->second;

	auto& glue = get_glue();
	if (auto window = glue.get_window(tp.surface)) {
		window->gui.send_mouse_button(
			false, // is_down
			tp.pos,
			ruis::mouse_button::left,
			tp.ruis_id
		);
	}

	self.touch_points.erase(i);
}

void wayland_touch_wrapper::wl_touch_motion(
	void* data, //
	wl_touch* touch,
	uint32_t time,
	int32_t id,
	wl_fixed_t x,
	wl_fixed_t y
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch motion event" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_touch_wrapper*>(data);
	utki::assert(self.touch == touch, SL);

	auto i = self.touch_points.find(id);
	utki::assert(i != self.touch_points.end(), SL);

	touch_point& tp = i->second;

	auto& glue = get_glue();
	if (auto window = glue.get_window(tp.surface)) {
		ruis::vector2 pos( //
			ruis::real(wl_fixed_to_double(x)),
			ruis::real(wl_fixed_to_double(y))
		);
		pos = round(pos * window->ruis_native_window.get().get_scale());

		tp.pos = pos;

		window->gui.send_mouse_move(
			tp.pos, //
			tp.ruis_id
		);
	}
}

// sent when compositor desides that touch gesture is going on, so
// all touch points become invalid and must be cancelled. No further
// events will be sent by wayland for current touch points.
void wayland_touch_wrapper::wl_touch_cancel(
	void* data, //
	wl_touch* touch
)
{
	utki::log_debug([](auto& o) {
		o << "wayland: touch cancel event" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_touch_wrapper*>(data);
	utki::assert(self.touch == touch, SL);

	auto& glue = get_glue();

	// send out-of-window button-up events fro all touch points
	for (const auto& pair : self.touch_points) {
		const auto& tp = pair.second;

		auto window = glue.get_window(tp.surface);
		if (!window) {
			continue;
		}

		window->gui.send_mouse_button(
			false, // is_down
			{-1, -1},
			ruis::mouse_button::left,
			tp.ruis_id
		);
	}

	self.touch_points.clear();
}
