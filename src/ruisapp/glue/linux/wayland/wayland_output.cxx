#include "wayland_output.hxx"

void wayland_output_wrapper::wl_output_geometry(
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
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_output_wrapper*>(data);

	self.position = {uint32_t(x), uint32_t(y)};
	self.physical_size_mm = {uint32_t(physical_width), uint32_t(physical_height)};

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ")" << '\n' //
		  << "  physical_size_mm = " << self.physical_size_mm << '\n' //
		  << "  make = " << make << '\n' //
		  << "  model = " << model << std::endl;
	});

	// TODO: is it needed to notify about outputs changed? Aren't wayland surfaces supposed to receive enter output events?

	// if (!application_constructed) {
	// 	// unable to obtain window_wrapper object before application is constructed,
	// 	// cannot do more without window_wrapper object
	// 	utki::log_debug([](auto& o) {
	// 		o << "  called within application constructor" << std::endl;
	// 	});
	// 	return;
	// }

	// auto& ww = get_impl(ruisapp::application::inst());
	// ww.notify_outputs_changed();
}

void wayland_output_wrapper::wl_output_mode(
	void* data, //
	struct wl_output* wl_output,
	uint32_t flags,
	int32_t width,
	int32_t height,
	int32_t refresh
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_output_wrapper*>(data);

	self.resolution = {uint32_t(width), uint32_t(height)};

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ") resolution = " << self.resolution << std::endl;
	});

	// TODO: do we need to notify about outputs changed in this case?

	// if (!application_constructed) {
	// 	// unable to obtain window_wrapper object before application is constructed,
	// 	// cannot do more without window_wrapper object
	// 	utki::log_debug([](auto& o) {
	// 		o << "  called within application constructor" << std::endl;
	// 	});
	// 	return;
	// }

	// auto& ww = get_impl(ruisapp::application::inst());
	// ww.notify_outputs_changed();
}

void wayland_output_wrapper::wl_output_scale(
	void* data, //
	struct wl_output* wl_output,
	int32_t factor
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_output_wrapper*>(data);

	self.scale = uint32_t(std::max(factor, 1));

	utki::log_debug([&](auto& o) {
		o << "output(" << self.id << ") scale = " << self.scale << std::endl;
	});

	// TODO: notify about outputs changed

	// if (!application_constructed) {
	// 	// unable to obtain window_wrapper object before application is constructed,
	// 	// cannot do more without window_wrapper object
	// 	utki::log_debug([](auto& o) {
	// 		o << "  called within application constructor" << std::endl;
	// 	});
	// 	return;
	// }

	// auto& ww = get_impl(ruisapp::application::inst());
	// ww.notify_outputs_changed();
}
