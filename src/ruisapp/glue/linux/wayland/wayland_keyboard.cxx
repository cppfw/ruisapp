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

#include "wayland_keyboard.hxx"

#include "application.hxx"

void wayland_keyboard_wrapper::wl_keyboard_enter(
	void* data, //
	wl_keyboard* keyboard,
	uint32_t serial,
	wl_surface* surface,
	wl_array* keys
)
{
	utki::log_debug([](auto& o) {
		o << "keyboard enter" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_keyboard_wrapper*>(data);

	// previous focused surface should leave focus first
	utki::assert(!self.focused_surface, SL);
	self.focused_surface = surface;

	auto& glue = get_glue();

	utki::assert(surface, SL);
	auto window = glue.get_window(surface);
	if (!window) {
		utki::logcat_debug("wayland_keyboard_wrapper::wl_keyboard_enter(): window no t found", '\n');
		return;
	}
	auto& win = *window;

	utki::logcat_debug("  window sequence_number: ", win.ruis_native_window.get().sequence_number, '\n');

	// notify ruis about pressed keys
	utki::assert(keys, SL);
	utki::assert(keys->size % sizeof(uint32_t) == 0, SL);
	for (auto key : utki::make_span(
			 static_cast<uint32_t*>(keys->data), //
			 keys->size / sizeof(uint32_t)
		 ))
	{
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
		ruis::key ruis_key = key_code_map[std::uint8_t(key)];
		win.gui.send_key(
			ruis::button_action::press, //
			ruis_key
		);
	}
}

void wayland_keyboard_wrapper::wl_keyboard_leave(
	void* data, //
	wl_keyboard* keyboard,
	uint32_t serial,
	wl_surface* surface
)
{
	utki::log_debug([](auto& o) {
		o << "keyboard leave" << std::endl;
	});

	utki::assert(data, SL);
	auto& self = *static_cast<wayland_keyboard_wrapper*>(data);

	if (!surface) {
		// This should not happen according to Wayland protocol, but it happens on practice.
		// For example when Wayland window which had keyboard focus is destroyed.
		// Just reset currently focused surface.
		self.focused_surface = nullptr;
		return;
	}

	auto& glue = get_glue();

	auto window = glue.get_window(surface);
	if (!window) {
		utki::logcat_debug("wayland_keyboard_wrapper::wl_keyboard_leave(): window not found", '\n');
		return;
	}
	auto& win = *window;

	utki::logcat_debug("  window sequence_number: ", win.ruis_native_window.get().sequence_number, '\n');

	utki::assert(self.focused_surface == surface, SL);
	self.focused_surface = nullptr;

	// TODO: send key releases

	// TODO: unfocus focused widget
}

void wayland_keyboard_wrapper::wl_keyboard_key(
	void* data, //
	wl_keyboard* keyboard,
	uint32_t serial,
	uint32_t time,
	uint32_t key,
	uint32_t state
)
{
	utki::assert(data, SL);
	auto& self = *static_cast<wayland_keyboard_wrapper*>(data);

	auto& glue = get_glue();
	auto window = glue.get_window(self.focused_surface);
	if (!window) {
		utki::log_debug([](auto& o) {
			o << "wayland_keyboard_wrapper::wl_keyboard_key(): no focused surface, ignore key event" << std::endl;
		});
		return;
	}

	auto& win = *window;

	bool is_pressed = state == WL_KEYBOARD_KEY_STATE_PRESSED;

	// std::cout << "keyboard key = " << key << ", pressed = " << (state == WL_KEYBOARD_KEY_STATE_PRESSED)
	// 		  << std::endl;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
	ruis::key ruis_key = key_code_map[std::uint8_t(key)];

	win.gui.send_key(
		is_pressed ? ruis::button_action::press : ruis::button_action::release, //
		ruis_key
	);

	class unicode_provider : public ruis::gui::input_string_provider
	{
		uint32_t key;
		// NOLINTNEXTLINE(clang-analyzer-webkit.NoUncountedMemberChecker, "false-positive")
		xkb_wrapper& xkb;

	public:
		unicode_provider(
			uint32_t key, //
			xkb_wrapper& xkb
		) :
			key(key),
			xkb(xkb)
		{}

		std::u32string get() const override
		{
			constexpr auto buf_size = 128;
			std::array<char, buf_size> buf{};

			constexpr auto wayland_linux_keycode_offset = 8;
			uint32_t keycode = this->key + wayland_linux_keycode_offset;
			ASSERT(this->xkb.state)

			xkb_state_key_get_utf8(
				this->xkb.state, //
				keycode,
				buf.data(),
				buf.size() - 1
			);

			buf.back() = '\0';

			return utki::to_utf32(buf.data());
		}
	};

	if (is_pressed) {
		win.gui.send_character_input(
			unicode_provider(
				key, //
				self.xkb
			), //
			ruis_key
		);
	}
}
