#pragma once

#include <stdexcept>

#include <ruis/gui.hpp>
#include <sys/mman.h>
#include <utki/debug.hpp>
#include <utki/unicode.hpp>
#include <utki/utility.hpp>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "key_code_map.hxx"

namespace {
class wayland_keyboard_wrapper
{
	unsigned num_connected = 0;

	wl_keyboard* keyboard = nullptr;

	wl_surface* focused_surface = nullptr;

	struct xkb_wrapper {
		xkb_context* context;
		xkb_keymap* keymap = nullptr;
		xkb_state* state = nullptr;

		xkb_wrapper() :
			context(xkb_context_new(XKB_CONTEXT_NO_FLAGS))
		{
			if (!this->context) {
				throw std::runtime_error("could not create xkb context");
			}
		}

		xkb_wrapper(const xkb_wrapper&) = delete;
		xkb_wrapper& operator=(const xkb_wrapper&) = delete;

		xkb_wrapper(xkb_wrapper&&) = delete;
		xkb_wrapper& operator=(xkb_wrapper&&) = delete;

		~xkb_wrapper()
		{
			this->set(nullptr, nullptr);
			xkb_context_unref(this->context);
		}

		void set(
			xkb_state* state, //
			xkb_keymap* keymap
		) noexcept
		{
			xkb_state_unref(this->state);
			xkb_keymap_unref(this->keymap);
			this->state = state;
			this->keymap = keymap;
		}
	} xkb;

	static void wl_keyboard_keymap(
		void* data, //
		wl_keyboard* keyboard,
		uint32_t format,
		int32_t fd,
		uint32_t size
	)
	{
		utki::assert(data, SL);
		auto& self = *static_cast<wayland_keyboard_wrapper*>(data);

		// std::cout << "keymap" << std::endl;

		utki::assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, SL);

		auto map_shm = mmap(
			nullptr, //
			size,
			PROT_READ,
			MAP_SHARED,
			fd,
			0
		);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, "using C API")
		if (map_shm == MAP_FAILED) {
			throw std::runtime_error("could not map shared memory to read wayland keymap");
		}

		utki::scope_exit scope_exit_mmap([&]() {
			munmap(
				map_shm, //
				size
			);
			close(fd);
		});

		xkb_keymap* keymap = xkb_keymap_new_from_string(
			self.xkb.context, //
			static_cast<const char*>(map_shm),
			XKB_KEYMAP_FORMAT_TEXT_V1,
			XKB_KEYMAP_COMPILE_NO_FLAGS
		);
		if (!keymap) {
			throw std::runtime_error("could not create xkb_keymap from string");
		}
		utki::scope_exit scope_exit_keymap([&]() {
			xkb_keymap_unref(keymap);
		});

		xkb_state* state = xkb_state_new(keymap);
		if (!state) {
			throw std::runtime_error("could not create xkb_state for keymap");
		}
		utki::scope_exit scope_exit_state([&]() {
			xkb_state_unref(state);
		});

		self.xkb.set(state, keymap);

		scope_exit_state.release();
		scope_exit_keymap.release();
	}

	static void wl_keyboard_enter(
		void* data, //
		wl_keyboard* keyboard,
		uint32_t serial,
		wl_surface* surface,
		wl_array* keys
	);

	static void wl_keyboard_leave(
		void* data, //
		wl_keyboard* keyboard,
		uint32_t serial,
		wl_surface* surface
	);

	static void wl_keyboard_key(
		void* data, //
		wl_keyboard* keyboard,
		uint32_t serial,
		uint32_t time,
		uint32_t key,
		uint32_t state
	);

	static void wl_keyboard_modifiers(
		void* data, //
		wl_keyboard* keyboard,
		uint32_t serial,
		uint32_t mods_depressed,
		uint32_t mods_latched,
		uint32_t mods_locked,
		uint32_t group
	)
	{
		utki::assert(data, SL);
		auto& self = *static_cast<wayland_keyboard_wrapper*>(data);

		// std::cout << "modifiers" << std::endl;

		xkb_state_update_mask(
			self.xkb.state, //
			mods_depressed,
			mods_latched,
			mods_locked,
			0,
			0,
			group
		);
	}

	static void wl_keyboard_repeat_info(
		void* data, //
		wl_keyboard* keyboard,
		int32_t rate,
		int32_t delay
	)
	{
		utki::log_debug([](auto& o) {
			o << "repeat info" << std::endl;
		});
	}

	constexpr static const wl_keyboard_listener listener = {
		.keymap = &wl_keyboard_keymap,
		.enter = &wl_keyboard_enter,
		.leave = &wl_keyboard_leave,
		.key = &wl_keyboard_key,
		.modifiers = &wl_keyboard_modifiers,
		.repeat_info = &wl_keyboard_repeat_info,
	};

public:
	void connect(wl_seat* seat)
	{
		++this->num_connected;

		if (this->num_connected > 1) {
			// already connected
			utki::assert(this->keyboard, SL);
			return;
		}

		this->keyboard = wl_seat_get_keyboard(seat);
		if (!this->keyboard) {
			this->num_connected = 0;
			throw std::runtime_error("could not get wayland keyboard interface");
		}

		if (wl_keyboard_add_listener(
				this->keyboard, //
				&listener,
				this
			) != 0)
		{
			wl_keyboard_release(this->keyboard);
			this->num_connected = 0;
			throw std::runtime_error("could not add listener to wayland keyboard interface");
		}
	}

	void disconnect() noexcept
	{
		if (this->num_connected == 0) {
			// no keyboards connected
			utki::assert(!this->keyboard, SL);
			return;
		}

		utki::assert(this->keyboard, SL);

		--this->num_connected;

		if (this->num_connected == 0) {
			wl_keyboard_release(this->keyboard);
			this->keyboard = nullptr;
		}
	}

	wayland_keyboard_wrapper() = default;

	wayland_keyboard_wrapper(const wayland_keyboard_wrapper&) = delete;
	wayland_keyboard_wrapper& operator=(const wayland_keyboard_wrapper&) = delete;

	wayland_keyboard_wrapper(wayland_keyboard_wrapper&&) = delete;
	wayland_keyboard_wrapper& operator=(wayland_keyboard_wrapper&&) = delete;

	~wayland_keyboard_wrapper()
	{
		if (this->keyboard) {
			if (wl_keyboard_get_version(this->keyboard) >= WL_KEYBOARD_RELEASE_SINCE_VERSION) {
				wl_keyboard_release(this->keyboard);
			} else {
				wl_keyboard_destroy(this->keyboard);
			}
		}
	}
};
} // namespace
