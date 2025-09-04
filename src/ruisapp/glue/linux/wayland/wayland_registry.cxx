#include "wayland_registry.hxx"

wayland_registry_wrapper::wayland_registry_wrapper(wayland_display_wrapper& wayland_display) :
	registry(wl_display_get_registry(wayland_display.display))
{
	if (!this->registry) {
		throw std::runtime_error("could not create wayland registry");
	}
	utki::scope_exit registry_scope_exit([this]() {
		this->destroy();
	});

	wl_registry_add_listener(
		this->registry, //
		&listener,
		this
	);

	// this will call the attached listener's global_registry_handler for all currently available wayland interfaces
	wl_display_roundtrip(wayland_display.display);
	wl_display_dispatch_pending(wayland_display.display);

	// std::cout << "registry events dispatched" << std::endl;

	// at this point we should have needed object ids set by global registry handler

	if (!this->compositor_name.has_value()) {
		throw std::runtime_error("could not find wayland compositor");
	}

	if (!this->wm_base_name.has_value()) {
		throw std::runtime_error("could not find xdg_shell");
	}

	if (!this->seat_name.has_value()) {
		utki::log_debug([](auto& o) {
			o << "WARNING: wayland system has no seat" << std::endl;
		});
	}

	if (!this->shm_name.has_value()) {
		throw std::runtime_error("could not find wl_shm");
	}

	registry_scope_exit.release();

	// std::cout << "registry constructed" << std::endl;
}

void wayland_registry_wrapper::wl_registry_global(
	void* data, //
	wl_registry* registry,
	uint32_t name,
	const char* interface,
	uint32_t version
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_registry_wrapper*>(data);

	utki::log_debug([&](auto& o) {
		o << "got a registry event for: " << interface << ", name = " << name << ", version = " << version << std::endl;
	});

	using namespace std::string_view_literals;

	if (std::string_view(interface) == "wl_compositor"sv) {
		self.compositor_name = {
			.name = name, //
			.version = version
		};
	} else if (std::string_view(interface) == xdg_wm_base_interface.name) {
		self.wm_base_name = {
			.name = name, //
			.version = version
		};
	} else if (std::string_view(interface) == "wl_seat"sv) {
		self.seat_name = {
			.name = name, //
			.version = version
		};
	} else if (std::string_view(interface) == "wl_shm"sv) {
		self.shm_name = {
			.name = name, //
			.version = version
		};
	} else if (std::string_view(interface) == "wl_output"sv) {
		utki::assert(self.registry, SL);
		self.outputs.emplace_back(
			*self.registry, //
			name,
			version
		);
	}

	// std::cout << "exit from registry event" << std::endl;
}

void wayland_registry_wrapper::wl_registry_global_remove(
	void* data, //
	wl_registry* registry,
	uint32_t name
)
{
	utki::log_debug([&](auto& o) {
		o << "got a registry losing event, name = " << name << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_registry_wrapper*>(data);

	// check if removed object is a wl_output
	if (auto i = std::find_if(
			self.outputs.begin(),
			self.outputs.end(),
			[&](const auto& o) {
				return o.wayland_global_object_name == name;
			}
		);
		i != self.outputs.end())
	{
		self.outputs.erase(i);
		utki::log_debug([&](auto& o) {
			o << "output removed, name = " << name << std::endl;
		});
		return;
	}
}
