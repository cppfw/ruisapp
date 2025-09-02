#include "wayland_surface.hxx"

#include "application.hxx"

void wayland_surface_wrapper::wl_surface_enter(
	void* data, //
	wl_surface* surface,
	wl_output* output
)
{
	utki::log_debug([&](auto& o) {
		o << "surface enters output(" << get_output_id(output) << ")" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_surface_wrapper*>(data);
	utki::assert(surface == self.surface, SL);

	utki::assert(self.wayland_outputs.find(output) == self.wayland_outputs.end(), SL);

	self.wayland_outputs.insert(output);

	auto& glue = get_glue();

	auto window = glue.get_window(self.surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_surface_wrapper::wl_surface_enter(): no window for given surface, ignoring enter output event"
			  << std::endl;
		});
		return;
	}

	window->notify_outputs_changed();
}

void wayland_surface_wrapper::wl_surface_leave(
	void* data, //
	wl_surface* surface,
	wl_output* output
)
{
	utki::log_debug([&](auto& o) {
		o << "surface leaves output(" << get_output_id(output) << ")" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_surface_wrapper*>(data);
	utki::assert(surface == self.surface, SL);

	utki::assert(self.wayland_outputs.find(output) != self.wayland_outputs.end(), SL);

	self.wayland_outputs.erase(output);

	auto& glue = get_glue();

	auto window = glue.get_window(self.surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_surface_wrapper::wl_surface_leave(): no window for given surface, ignoring leave output event"
			  << std::endl;
		});
		return;
	}

	window->notify_outputs_changed();
}
