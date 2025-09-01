#pragma once

#include <map>
#include <optional>
#include <string_view>

#include <utki/debug.hpp>
#include <utki/utility.hpp>
#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>

#include "wayland_display.hxx"
#include "wayland_output.hxx"

namespace {
struct wayland_registry_wrapper {
	wl_registry* registry;

	struct interface_id {
		uint32_t id;
		uint32_t version;
	};

	std::optional<interface_id> compositor_id;
	std::optional<interface_id> wm_base_id;
	std::optional<interface_id> shm_id;
	std::optional<interface_id> seat_id;

	std::map<uint32_t, wayland_output_wrapper> outputs;

	static void wl_registry_global(
		void* data,
		wl_registry* registry,
		uint32_t id,
		const char* interface,
		uint32_t version
	)
	{
		utki::assert(data, SL);
		auto& self = *static_cast<wayland_registry_wrapper*>(data);

		utki::log_debug([&](auto& o) {
			o << "got a registry event for: " << interface << ", id = " << id << std::endl;
		});

		using namespace std::string_view_literals;

		if (std::string_view(interface) == "wl_compositor"sv) {
			self.compositor_id = {
				.id = id, //
				.version = version
			};
		} else if (std::string_view(interface) == xdg_wm_base_interface.name) {
			self.wm_base_id = {
				.id = id, //
				.version = version
			};
		} else if (std::string_view(interface) == "wl_seat"sv) {
			self.seat_id = {
				.id = id, //
				.version = version
			};
		} else if (std::string_view(interface) == "wl_shm"sv) {
			self.shm_id = {
				.id = id, //
				.version = version
			};
		} else if (std::string_view(interface) == "wl_output"sv) {
			utki::assert(self.registry, SL);
			self.outputs.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(id),
				std::forward_as_tuple(
					*self.registry, //
					id,
					version
				)
			);
		}

		// std::cout << "exit from registry event" << std::endl;
	}

	static void wl_registry_global_remove(
		void* data, //
		wl_registry* registry,
		uint32_t id
	)
	{
		utki::log_debug([&](auto& o) {
			o << "got a registry losing event, id = " << id << std::endl;
		});

		utki::assert(data, SL);
		auto& self = *static_cast<wayland_registry_wrapper*>(data);

		// check if removed object is a wl_output
		if (auto i = self.outputs.find(id); i != self.outputs.end()) {
			self.outputs.erase(i);
			utki::log_debug([&](auto& o) {
				o << "output removed, id = " << id << std::endl;
			});
			return;
		}
	}

	constexpr static const wl_registry_listener listener = {
		.global = &wl_registry_global,
		.global_remove = wl_registry_global_remove
	};

	wayland_registry_wrapper(wayland_display_wrapper& wayland_display) :
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

		// this will call the attached listener's global_registry_handler
		wl_display_roundtrip(wayland_display.display);
		wl_display_dispatch_pending(wayland_display.display);

		// std::cout << "registry events dispatched" << std::endl;

		// at this point we should have needed object ids set by global registry handler

		if (!this->compositor_id.has_value()) {
			throw std::runtime_error("could not find wayland compositor");
		}

		if (!this->wm_base_id.has_value()) {
			throw std::runtime_error("could not find xdg_shell");
		}

		if (!this->seat_id.has_value()) {
			utki::log_debug([](auto& o) {
				o << "WARNING: wayland system has no seat" << std::endl;
			});
		}

		if (!this->shm_id.has_value()) {
			throw std::runtime_error("could not find wl_shm");
		}

		registry_scope_exit.release();

		// std::cout << "registry constructed" << std::endl;
	}

	wayland_registry_wrapper(const wayland_registry_wrapper&) = delete;
	wayland_registry_wrapper& operator=(const wayland_registry_wrapper&) = delete;

	wayland_registry_wrapper(wayland_registry_wrapper&&) = delete;
	wayland_registry_wrapper& operator=(wayland_registry_wrapper&&) = delete;

	~wayland_registry_wrapper()
	{
		this->destroy();
	}

private:
	void destroy()
	{
		wl_registry_destroy(this->registry);
	}
};
} // namespace
