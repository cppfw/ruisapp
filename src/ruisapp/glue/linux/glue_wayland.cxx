/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2024  Ivan Gagis <igagis@gmail.com>

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

#include <atomic>

#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <papki/fs_file.hpp>
#include <utki/destructable.hpp>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glew.h>
// #	include <GL/glx.h>
#	include <ruis/render/opengl/renderer.hpp>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <GLES2/gl2.h>
#	include <ruis/render/opengles/renderer.hpp>

#else
#	error "Unknown graphics API"
#endif

#include <xdg-shell-client-protocol.h>

#include "../../application.hpp"
#include "../friend_accessors.cxx" // NOLINT(bugprone-suspicious-include)
#include "../unix_common.cxx" // NOLINT(bugprone-suspicious-include)

using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
ruis::mouse_button button_number_to_enum(uint32_t number)
{
	// from wayland's comments:
	// The button is a button code as defined in the Linux kernel's
	// linux/input-event-codes.h header file, e.g. BTN_LEFT.

	switch (number) {
		case 0x110: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::left;
		default:
		case 0x112: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::middle;
		case 0x111: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::right;

			// TODO: handle

			// #define BTN_SIDE		0x113
			// #define BTN_EXTRA		0x114
			// #define BTN_FORWARD		0x115
			// #define BTN_BACK		0x116
			// #define BTN_TASK		0x117
	}
}
} // namespace

namespace {
const std::array<ruis::key, size_t(std::numeric_limits<uint8_t>::max()) + 1> key_code_map = {
	{//
	 ruis::key::unknown,
	 ruis::key::escape, // 9
	 ruis::key::one, // 10
	 ruis::key::two, // 11
	 ruis::key::three, // 12
	 ruis::key::four, // 13
	 ruis::key::five, // 14
	 ruis::key::six, // 15
	 ruis::key::seven, // 16
	 ruis::key::eight, // 17
	 ruis::key::nine, // 18
	 ruis::key::zero, // 19
	 ruis::key::minus, // 20
	 ruis::key::equals, // 21
	 ruis::key::backspace, // 22
	 ruis::key::tabulator, // 23
	 ruis::key::q, // 24
	 ruis::key::w, // 25
	 ruis::key::e, // 26
	 ruis::key::r, // 27
	 ruis::key::t, // 28
	 ruis::key::y, // 29
	 ruis::key::u, // 30
	 ruis::key::i, // 31
	 ruis::key::o, // 32
	 ruis::key::p, // 33
	 ruis::key::left_square_bracket, // 34
	 ruis::key::right_square_bracket, // 35
	 ruis::key::enter, // 36
	 ruis::key::left_control, // 37
	 ruis::key::a, // 38
	 ruis::key::s, // 39
	 ruis::key::d, // 40
	 ruis::key::f, // 41
	 ruis::key::g, // 42
	 ruis::key::h, // 43
	 ruis::key::j, // 44
	 ruis::key::k, // 45
	 ruis::key::l, // 46
	 ruis::key::semicolon, // 47
	 ruis::key::apostrophe, // 48
	 ruis::key::grave, // 49
	 ruis::key::left_shift, // 50
	 ruis::key::backslash, // 51
	 ruis::key::z, // 52
	 ruis::key::x, // 53
	 ruis::key::c, // 54
	 ruis::key::v, // 55
	 ruis::key::b, // 56
	 ruis::key::n, // 57
	 ruis::key::m, // 58
	 ruis::key::comma, // 59
	 ruis::key::period, // 60
	 ruis::key::slash, // 61
	 ruis::key::right_shift, // 62
	 ruis::key::unknown,
	 ruis::key::left_alt, // 64
	 ruis::key::space, // 65
	 ruis::key::capslock, // 66
	 ruis::key::f1, // 67
	 ruis::key::f2, // 68
	 ruis::key::f3, // 69
	 ruis::key::f4, // 70
	 ruis::key::f5, // 71
	 ruis::key::f6, // 72
	 ruis::key::f7, // 73
	 ruis::key::f8, // 74
	 ruis::key::f9, // 75
	 ruis::key::f10, // 76
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::f11, // 95
	 ruis::key::f12, // 96
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::right_control, // 105
	 ruis::key::unknown,
	 ruis::key::print_screen, // 107
	 ruis::key::right_alt, // 108
	 ruis::key::unknown,
	 ruis::key::home, // 110
	 ruis::key::arrow_up, // 111
	 ruis::key::page_up, // 112
	 ruis::key::arrow_left, // 113
	 ruis::key::arrow_right, // 114
	 ruis::key::end, // 115
	 ruis::key::arrow_down, // 116
	 ruis::key::page_down, // 117
	 ruis::key::insert, // 118
	 ruis::key::deletion, // 119
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::pause, // 127
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::left_command, // 133
	 ruis::key::unknown,
	 ruis::key::menu, // 135
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown,
	 ruis::key::unknown
	}
};
} // namespace

namespace {
struct keyboard_wrapper {
	unsigned num_connected = 0;

	wl_keyboard* keyboard = nullptr;

	static void wl_keyboard_keymap(void* data, struct wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
	{
		// ASSERT(data)
		// auto& self = *static_cast<keyboard_wrapper*>(data);

		std::cout << "keymap" << std::endl;

		//    struct client_state *client_state = data;
		//    assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

		//    char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
		//    assert(map_shm != MAP_FAILED);

		//    struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(
		//                    client_state->xkb_context, map_shm,
		//                    XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
		//    munmap(map_shm, size);
		//    close(fd);

		//    struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
		//    xkb_keymap_unref(client_state->xkb_keymap);
		//    xkb_state_unref(client_state->xkb_state);
		//    client_state->xkb_keymap = xkb_keymap;
		//    client_state->xkb_state = xkb_state;
	}

	static void wl_keyboard_enter(
		void* data,
		struct wl_keyboard* keyboard,
		uint32_t serial,
		struct wl_surface* surface,
		struct wl_array* keys
	)
	{
		std::cout << "keyboard enter" << std::endl;
		//    struct client_state *client_state = data;
		//    fprintf(stderr, "keyboard enter; keys pressed are:\n");
		//    uint32_t *key;
		//    wl_array_for_each(key, keys) {
		//            char buf[128];
		//            xkb_keysym_t sym = xkb_state_key_get_one_sym(
		//                            client_state->xkb_state, *key + 8);
		//            xkb_keysym_get_name(sym, buf, sizeof(buf));
		//            fprintf(stderr, "sym: %-12s (%d), ", buf, sym);
		//            xkb_state_key_get_utf8(client_state->xkb_state,
		//                            *key + 8, buf, sizeof(buf));
		//            fprintf(stderr, "utf8: '%s'\n", buf);
		//    }
	}

	static void wl_keyboard_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface)
	{
		std::cout << "keyboard leave" << std::endl;
	}

	static void wl_keyboard_key(
		void* data,
		struct wl_keyboard* keyboard,
		uint32_t serial,
		uint32_t time,
		uint32_t key,
		uint32_t state
	)
	{
		// std::cout << "keyboard key = " << key << ", pressed = " << (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		// 		  << std::endl;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
		ruis::key ruis_key = key_code_map[std::uint8_t(key)];
		handle_key_event(ruisapp::inst(), state == WL_KEYBOARD_KEY_STATE_PRESSED, ruis_key);

		// TODO:
		// handle_character_input(*app, key_event_unicode_provider(ww.input_context, event), key);

		//    struct client_state *client_state = data;
		//    char buf[128];
		//    uint32_t keycode = key + 8;
		//    xkb_keysym_t sym = xkb_state_key_get_one_sym(
		//                    client_state->xkb_state, keycode);
		//    xkb_keysym_get_name(sym, buf, sizeof(buf));
		//    const char *action =
		//            state == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
		//    fprintf(stderr, "key %s: sym: %-12s (%d), ", action, buf, sym);
		//    xkb_state_key_get_utf8(client_state->xkb_state, keycode,
		//                    buf, sizeof(buf));
		//    fprintf(stderr, "utf8: '%s'\n", buf);
	}

	static void wl_keyboard_modifiers(
		void* data,
		struct wl_keyboard* keyboard,
		uint32_t serial,
		uint32_t mods_depressed,
		uint32_t mods_latched,
		uint32_t mods_locked,
		uint32_t group
	)
	{
		std::cout << "modifiers" << std::endl;
		// struct client_state* client_state = data;
		// xkb_state_update_mask(client_state->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
	}

	static void wl_keyboard_repeat_info(void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay)
	{
		std::cout << "repeat info" << std::endl;
	}

	constexpr static const wl_keyboard_listener listener = {
		.keymap = &wl_keyboard_keymap,
		.enter = &wl_keyboard_enter,
		.leave = &wl_keyboard_leave,
		.key = &wl_keyboard_key,
		.modifiers = &wl_keyboard_modifiers,
		.repeat_info = &wl_keyboard_repeat_info,
	};

	void connect(wl_seat* seat)
	{
		++this->num_connected;

		if (this->num_connected > 1) {
			// already connected
			ASSERT(this->keyboard)
			return;
		}

		this->keyboard = wl_seat_get_keyboard(seat);
		if (!this->keyboard) {
			this->num_connected = 0;
			throw std::runtime_error("could not get wayland keyboard interface");
		}

		if (wl_keyboard_add_listener(this->keyboard, &listener, this) != 0) {
			wl_keyboard_release(this->keyboard);
			this->num_connected = 0;
			throw std::runtime_error("could not add listener to wayland keyboard interface");
		}
	}

	void disconnect() noexcept
	{
		if (this->num_connected == 0) {
			// no keyboards connected
			ASSERT(!this->keyboard)
			return;
		}

		ASSERT(this->keyboard)

		--this->num_connected;

		if (this->num_connected == 0) {
			wl_keyboard_release(this->keyboard);
			this->keyboard = nullptr;
		}
	}

	keyboard_wrapper() = default;

	~keyboard_wrapper()
	{
		if (this->keyboard) {
			wl_keyboard_release(this->keyboard);
		}
	}
};

} // namespace

namespace {
struct pointer_wrapper {
	unsigned num_connected = 0;

	wl_pointer* pointer = nullptr;

	ruis::vector2 cur_pointer_pos{0, 0};

	static void wl_pointer_enter(
		void* data,
		struct wl_pointer* pointer,
		uint32_t serial,
		struct wl_surface* surface,
		wl_fixed_t x,
		wl_fixed_t y
	) //
	{
		// std::cout << "mouse enter: x,y = " << std::dec << x << ", " << y << std::endl;
		auto& self = *static_cast<pointer_wrapper*>(data);
		handle_mouse_hover(ruisapp::inst(), true, 0);
		self.cur_pointer_pos = ruis::vector2(wl_fixed_to_int(x), wl_fixed_to_int(y));
		handle_mouse_move(ruisapp::inst(), self.cur_pointer_pos, 0);
	}

	static void wl_pointer_motion(void* data, struct wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
	{
		// std::cout << "mouse move: x,y = " << std::dec << x << ", " << y << std::endl;
		auto& self = *static_cast<pointer_wrapper*>(data);
		self.cur_pointer_pos = ruis::vector2(wl_fixed_to_int(x), wl_fixed_to_int(y));
		handle_mouse_move(ruisapp::inst(), self.cur_pointer_pos, 0);
	}

	static void wl_pointer_button(
		void* data,
		struct wl_pointer* pointer,
		uint32_t serial,
		uint32_t time,
		uint32_t button,
		uint32_t state
	) //
	{
		// std::cout << "mouse button: " << std::hex << "0x" << button << ", state = " << "0x" << state <<
		// std::endl;
		auto& self = *static_cast<pointer_wrapper*>(data);
		handle_mouse_button(
			ruisapp::inst(),
			state == WL_POINTER_BUTTON_STATE_PRESSED,
			self.cur_pointer_pos,
			button_number_to_enum(button),
			0
		);
	}

	static void wl_pointer_axis(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
	{
		auto& self = *static_cast<pointer_wrapper*>(data);

		// we get +-10 for each mouse wheel step
		auto val = wl_fixed_to_int(value);

		// std::cout << "mouse axis: " << std::dec << axis << ", val = " << val << std::endl;

		for (unsigned i = 0; i != 2; ++i) {
			handle_mouse_button(
				ruisapp::inst(),
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
			[](void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface) {
				// std::cout << "mouse leave" << std::endl;
				handle_mouse_hover(ruisapp::inst(), false, 0);
			},
		.motion = &wl_pointer_motion,
		.button = &wl_pointer_button,
		.axis = &wl_pointer_axis,
		.frame =
			[](void* data, struct wl_pointer* pointer) {
				LOG([](auto& o) {
					o << "pointer frame" << std::endl;
				})
			},
		.axis_source =
			[](void* data, struct wl_pointer* pointer, uint32_t source) {
				LOG([&](auto& o) {
					o << "axis source: " << std::dec << source << std::endl;
				})
			},
		.axis_stop =
			[](void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis) {
				LOG([&](auto& o) {
					o << "axis stop: axis = " << std::dec << axis << std::endl;
				})
			},
		.axis_discrete =
			[](void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete) {
				LOG([&](auto& o) {
					o << "axis discrete: axis = " << std::dec << axis << ", discrete = " << discrete << std::endl;
				})
			}
	};

	void connect(wl_seat* seat)
	{
		++this->num_connected;

		if (this->num_connected > 1) {
			// already connected
			ASSERT(this->pointer)
			return;
		}

		this->pointer = wl_seat_get_pointer(seat);
		if (!this->pointer) {
			this->num_connected = 0;
			throw std::runtime_error("could not get wayland pointer interface");
		}

		if (wl_pointer_add_listener(this->pointer, &listener, this) != 0) {
			wl_pointer_release(this->pointer);
			this->num_connected = 0;
			throw std::runtime_error("could not add listener to wayland pointer interface");
		}
	}

	void disconnect() noexcept
	{
		if (this->num_connected == 0) {
			// no pointers connected
			ASSERT(!this->pointer)
			return;
		}

		ASSERT(this->pointer)

		--this->num_connected;

		if (this->num_connected == 0) {
			wl_pointer_release(this->pointer);
			this->pointer = nullptr;
		}
	}

	pointer_wrapper() = default;

	~pointer_wrapper()
	{
		if (this->pointer) {
			wl_pointer_release(this->pointer);
		}
	}
};
} // namespace

namespace {

struct window_wrapper;

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl);
window_wrapper& get_impl(application& app);

struct window_wrapper : public utki::destructable {
	std::atomic_bool quit_flag = false;

	nitki::queue ui_queue;

	struct display_wrapper {
		wl_display* disp;

		display_wrapper() :
			disp(wl_display_connect(nullptr))
		{
			if (!this->disp) {
				throw std::runtime_error("could not connect to wayland display");
			}
		}

		display_wrapper(const display_wrapper&) = delete;
		display_wrapper& operator=(const display_wrapper&) = delete;

		display_wrapper(display_wrapper&&) = delete;
		display_wrapper& operator=(display_wrapper&&) = delete;

		~display_wrapper()
		{
			wl_display_disconnect(this->disp);
		}
	} display;

	class wayland_waitable : public opros::waitable
	{
	public:
		wayland_waitable(display_wrapper& display) :
			opros::waitable([&]() {
				auto fd = wl_display_get_fd(display.disp);
				ASSERT(fd != 0)
				return fd;
			}())
		{}
	} waitable;

	struct registry_wrapper {
		wl_registry* reg;

		wl_compositor* compositor = nullptr;
		xdg_wm_base* wm_base = nullptr;
		wl_seat* seat = nullptr;
		wl_shm* shm = nullptr;

		pointer_wrapper pointer;
		keyboard_wrapper keyboard;

		constexpr static const xdg_wm_base_listener wm_base_listener = {
			.ping =
				[](void* data, xdg_wm_base* wm_base, uint32_t serial) {
					xdg_wm_base_pong(wm_base, serial);
				} //
		};

		static void wl_seat_capabilities(void* data, struct wl_seat* wl_seat, uint32_t capabilities)
		{
			LOG([&](auto& o) {
				o << "seat capabilities: " << std::hex << "0x" << capabilities << std::endl;
			})

			auto& self = *static_cast<registry_wrapper*>(data);

			bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

			if (have_pointer) {
				self.pointer.connect(self.seat);
			} else {
				self.pointer.disconnect();
			}

			bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

			if (have_keyboard) {
				self.keyboard.connect(self.seat);
			} else {
				self.keyboard.disconnect();
			}
		}

		constexpr static const wl_seat_listener seat_listener = {
			.capabilities = &wl_seat_capabilities,
			.name =
				[](void* data, struct wl_seat* seat, const char* name) {
					LOG([&](auto& o) {
						o << "seat name: " << name << std::endl;
					})
				} //
		};

		static void wl_registry_global(
			void* data,
			wl_registry* registry,
			uint32_t id,
			const char* interface,
			uint32_t version
		)
		{
			ASSERT(data)
			auto& self = *static_cast<registry_wrapper*>(data);

			LOG([&](auto& o) {
				o << "got a registry event for: " << interface << ", id = " << id << std::endl;
			});
			if (std::string_view(interface) == "wl_compositor"sv && !self.compositor) {
				void* compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
				ASSERT(compositor)
				self.compositor = static_cast<wl_compositor*>(compositor);
			} else if (std::string_view(interface) == xdg_wm_base_interface.name && !self.wm_base) {
				void* wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
				ASSERT(wm_base)
				self.wm_base = static_cast<xdg_wm_base*>(wm_base);
				xdg_wm_base_add_listener(self.wm_base, &wm_base_listener, nullptr);
			} else if (std::string_view(interface) == "wl_seat"sv && !self.seat) {
				void* seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
				ASSERT(seat)
				self.seat = static_cast<wl_seat*>(seat);
				wl_seat_add_listener(self.seat, &seat_listener, &self);
			} else if (std::string_view(interface) == "wl_shm"sv && !self.shm) {
				void* shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
				ASSERT(shm)
				self.shm = static_cast<wl_shm*>(shm);
			}
		}

		constexpr static const wl_registry_listener listener = {
			.global = &wl_registry_global,
			.global_remove =
				[](void* data, struct wl_registry* registry, uint32_t id) {
					LOG([&](auto& o) {
						o << "got a registry losing event, id = " << id << std::endl;
					});
					// we assume that compositor and shell objects will never be removed
				} //
		};

		registry_wrapper(display_wrapper& display) :
			reg(wl_display_get_registry(display.disp))
		{
			if (!this->reg) {
				throw std::runtime_error("could not create wayland registry");
			}
			utki::scope_exit registry_scope_exit([this]() {
				this->destroy();
			});

			wl_registry_add_listener(this->reg, &listener, this);

			// this will call the attached listener's global_registry_handler
			wl_display_roundtrip(display.disp);
			wl_display_dispatch(display.disp);

			// at this point we should have compositor and shell set by global_registry_handler

			if (!this->compositor) {
				throw std::runtime_error("could not find wayland compositor");
			}

			if (!this->wm_base) {
				throw std::runtime_error("could not find xdg_shell");
			}

			if (!this->seat) {
				throw std::runtime_error("could not find wl_seat");
			}

			if (!this->shm) {
				throw std::runtime_error("could not find wl_shm");
			}

			registry_scope_exit.release();
		}

		registry_wrapper(const registry_wrapper&) = delete;
		registry_wrapper& operator=(const registry_wrapper&) = delete;

		registry_wrapper(registry_wrapper&&) = delete;
		registry_wrapper& operator=(registry_wrapper&&) = delete;

		~registry_wrapper()
		{
			this->destroy();
		}

	private:
		void destroy()
		{
			if (this->shm) {
				wl_shm_destroy(this->shm);
			}
			if (this->seat) {
				wl_seat_destroy(this->seat);
			}
			if (this->wm_base) {
				xdg_wm_base_destroy(this->wm_base);
			}
			if (this->compositor) {
				wl_compositor_destroy(this->compositor);
			}
			wl_registry_destroy(this->reg);
		}
	} registry;

	struct region_wrapper {
		wl_region* reg;

		region_wrapper(registry_wrapper& registry) :
			reg(wl_compositor_create_region(registry.compositor))
		{
			if (!this->reg) {
				throw std::runtime_error("could not create wayland region");
			}
		}

		region_wrapper(const region_wrapper&) = delete;
		region_wrapper& operator=(const region_wrapper&) = delete;

		region_wrapper(region_wrapper&&) = delete;
		region_wrapper& operator=(region_wrapper&&) = delete;

		~region_wrapper()
		{
			wl_region_destroy(this->reg);
		}

		void add(r4::rectangle<int32_t> rect)
		{
			wl_region_add(this->reg, rect.p.x(), rect.p.y(), rect.d.x(), rect.d.y());
		}
	};

	struct surface_wrapper {
		wl_surface* sur;

		surface_wrapper(registry_wrapper& registry) :
			sur(wl_compositor_create_surface(registry.compositor))
		{
			if (!this->sur) {
				throw std::runtime_error("could not create wayland surface");
			}
		}

		surface_wrapper(const surface_wrapper&) = delete;
		surface_wrapper& operator=(const surface_wrapper&) = delete;

		surface_wrapper(surface_wrapper&&) = delete;
		surface_wrapper& operator=(surface_wrapper&&) = delete;

		~surface_wrapper()
		{
			wl_surface_destroy(this->sur);
		}

		void commit()
		{
			wl_surface_commit(this->sur);
		}

		void set_opaque_region(const region_wrapper& region)
		{
			wl_surface_set_opaque_region(this->sur, region.reg);
		}
	} surface;

	struct xdg_surface_wrapper {
		xdg_surface* xdg_sur;

		constexpr static const xdg_surface_listener listener = {
			.configure =
				[](void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
					xdg_surface_ack_configure(xdg_surface, serial);
				}, //
		};

		xdg_surface_wrapper(surface_wrapper& surface, registry_wrapper& registry) :
			xdg_sur(xdg_wm_base_get_xdg_surface(registry.wm_base, surface.sur))
		{
			if (!xdg_sur) {
				throw std::runtime_error("could not create wayland xdg surface");
			}

			xdg_surface_add_listener(this->xdg_sur, &listener, nullptr);
		}

		xdg_surface_wrapper(const xdg_surface_wrapper&) = delete;
		xdg_surface_wrapper& operator=(const xdg_surface_wrapper&) = delete;

		xdg_surface_wrapper(xdg_surface_wrapper&&) = delete;
		xdg_surface_wrapper& operator=(xdg_surface_wrapper&&) = delete;

		~xdg_surface_wrapper()
		{
			xdg_surface_destroy(this->xdg_sur);
		}
	} xdg_surface;

	struct toplevel_wrapper {
		xdg_toplevel* toplev;

		static void xdg_toplevel_configure(
			void* data,
			struct xdg_toplevel* xdg_toplevel,
			int32_t width,
			int32_t height,
			struct wl_array* states
		)
		{
			LOG([](auto& o) {
				o << "window configure" << std::endl;
			})

			// not a window geometry event, ignore
			if (width == 0 && height == 0) {
				return;
			}

			ASSERT(width >= 0)
			ASSERT(height >= 0)

			// window resized

			LOG([](auto& o) {
				o << "window resized" << std::endl;
			})

			auto& ww = get_impl(ruisapp::inst());

			wl_egl_window_resize(ww.egl_window.win, width, height, 0, 0);
			ww.surface.commit();

			update_window_rect(ruisapp::inst(), ruis::rect(0, {ruis::real(width), ruis::real(height)}));
		}

		static void xdg_toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel)
		{
			// window closed
			auto& ww = get_impl(ruisapp::inst());
			ww.quit_flag.store(true);
		}

		constexpr static const xdg_toplevel_listener listener = {
			.configure = xdg_toplevel_configure,
			.close = &xdg_toplevel_close,
		};

		toplevel_wrapper(surface_wrapper& surface, xdg_surface_wrapper& xdg_surface) :
			toplev(xdg_surface_get_toplevel(xdg_surface.xdg_sur))
		{
			if (!this->toplev) {
				throw std::runtime_error("could not get wayland xdg toplevel");
			}

			xdg_toplevel_set_title(this->toplev, "ruisapp wayland");
			xdg_toplevel_add_listener(this->toplev, &listener, nullptr);

			surface.commit();
		}

		toplevel_wrapper(const toplevel_wrapper&) = delete;
		toplevel_wrapper& operator=(const toplevel_wrapper&) = delete;

		toplevel_wrapper(toplevel_wrapper&&) = delete;
		toplevel_wrapper& operator=(toplevel_wrapper&&) = delete;

		~toplevel_wrapper()
		{
			xdg_toplevel_destroy(this->toplev);
		}
	} toplevel;

	region_wrapper region;

	struct egl_window_wrapper {
		wl_egl_window* win;

		egl_window_wrapper(surface_wrapper& surface, region_wrapper& region, r4::vector2<unsigned> dims)
		{
			region.add(r4::rectangle({0, 0}, dims.to<int32_t>()));
			surface.set_opaque_region(region);

			auto int_dims = dims.to<int>();

			// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
			this->win = wl_egl_window_create(surface.sur, int_dims.x(), int_dims.y());

			if (!this->win) {
				throw std::runtime_error("could not create wayland egl window");
			}
		}

		egl_window_wrapper(const egl_window_wrapper&) = delete;
		egl_window_wrapper& operator=(const egl_window_wrapper&) = delete;

		egl_window_wrapper(egl_window_wrapper&&) = delete;
		egl_window_wrapper& operator=(egl_window_wrapper&&) = delete;

		~egl_window_wrapper()
		{
			wl_egl_window_destroy(this->win);
		}
	} egl_window;

	struct egl_context_wrapper {
		EGLDisplay egl_display;
		EGLSurface egl_surface;
		EGLContext egl_context;

		egl_context_wrapper(
			const display_wrapper& display,
			const egl_window_wrapper& egl_window,
			const window_params& wp
		) :
			egl_display(eglGetDisplay(display.disp))
		{
			if (this->egl_display == EGL_NO_DISPLAY) {
				throw std::runtime_error("could not open EGL display");
			}

			utki::scope_exit scope_exit_egl_display([this]() {
				eglTerminate(this->egl_display);
			});

			if (!eglInitialize(this->egl_display, nullptr, nullptr)) {
				throw std::runtime_error("could not initialize EGL");
			}

			EGLConfig egl_config = nullptr;
			{
				// Here specify the attributes of the desired configuration.
				// Below, we select an EGLConfig with at least 8 bits per color
				// component compatible with on-screen windows.
				const std::array<EGLint, 15> attribs = {
					EGL_SURFACE_TYPE,
					EGL_WINDOW_BIT,
					EGL_RENDERABLE_TYPE,
					EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT,
					EGL_BLUE_SIZE,
					8,
					EGL_GREEN_SIZE,
					8,
					EGL_RED_SIZE,
					8,
					EGL_DEPTH_SIZE,
					wp.buffers.get(window_params::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
					EGL_STENCIL_SIZE,
					wp.buffers.get(window_params::buffer::stencil) ? utki::byte_bits : 0,
					EGL_NONE
				};

				// Here, the application chooses the configuration it desires. In this
				// sample, we have a very simplified selection process, where we pick
				// the first EGLConfig that matches our criteria.
				EGLint num_configs = 0;
				eglChooseConfig(this->egl_display, attribs.data(), &egl_config, 1, &num_configs);
				if (num_configs <= 0) {
					throw std::runtime_error("eglChooseConfig() failed, no matching config found");
				}
			}

			if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
				throw std::runtime_error("eglBindApi(OpenGL ES) failed");
			}

			this->egl_surface = eglCreateWindowSurface(this->egl_display, egl_config, egl_window.win, nullptr);
			if (this->egl_surface == EGL_NO_SURFACE) {
				throw std::runtime_error("could not create EGL window surface");
			}

			utki::scope_exit scope_exit_egl_window_surface([this]() {
				eglDestroySurface(this->egl_display, this->egl_surface);
			});

			auto graphics_api_version = [&ver = wp.graphics_api_version]() {
				if (ver.to_uint32_t() == 0) {
					// default OpenGL ES version is 2.0
					return utki::version_duplet{
						.major = 2, //
						.minor = 0
					};
				}
				return ver;
			}();

			{
				constexpr auto attrs_array_size = 5;
				std::array<EGLint, attrs_array_size> context_attrs = {
					EGL_CONTEXT_MAJOR_VERSION,
					graphics_api_version.major,
					EGL_CONTEXT_MINOR_VERSION,
					graphics_api_version.minor,
					EGL_NONE
				};

				this->egl_context =
					eglCreateContext(this->egl_display, egl_config, EGL_NO_CONTEXT, context_attrs.data());
				if (this->egl_context == EGL_NO_CONTEXT) {
					throw std::runtime_error("could not create EGL context");
				}
			}

			utki::scope_exit scope_exit_egl_context([this]() {
				// unset current context
				eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

				eglDestroyContext(this->egl_display, this->egl_context);
			});

			if (eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context) == EGL_FALSE)
			{
				throw std::runtime_error("eglMakeCurrent() failed");
			}

			// disable v-sync
			// if (eglSwapInterval(this->egl_display, 0) != EGL_TRUE) {
			// 	throw std::runtime_error("eglSwapInterval() failed");
			// }

			scope_exit_egl_context.release();
			scope_exit_egl_window_surface.release();
			scope_exit_egl_display.release();
		}

		egl_context_wrapper(const egl_context_wrapper&) = delete;
		egl_context_wrapper& operator=(const egl_context_wrapper&) = delete;

		egl_context_wrapper(egl_context_wrapper&&) = delete;
		egl_context_wrapper& operator=(egl_context_wrapper&&) = delete;

		~egl_context_wrapper()
		{
			// unset current context
			eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

			eglDestroyContext(this->egl_display, this->egl_context);
			eglDestroySurface(this->egl_display, this->egl_surface);
			eglTerminate(this->egl_display);
		}

		void swap_frame_buffers()
		{
			eglSwapBuffers(this->egl_display, this->egl_surface);
		}
	} egl_context;

	window_wrapper(const window_params& wp) :
		waitable(this->display),
		registry(this->display),
		surface(this->registry),
		xdg_surface(this->surface, this->registry),
		toplevel(this->surface, this->xdg_surface),
		region(this->registry),
		egl_window(this->surface, this->region, wp.dims),
		egl_context(this->display, this->egl_window, wp)
	{
		// WORKAROUND: the following calls are figured out by trial and error. Without those the wayland main loop
		//             either gets stuck on waiting for events and no events come and window is not shown, or
		//             some call related to wayland events queue fails with error.
		// no idea why roundtrip is needed, perhaps to configure the xdg surface before actually drawing to it
		wl_display_roundtrip(this->display.disp);
		// no idea why initial buffer swap is needed, perhaps it moves the window configure procedure forward somehow
		this->egl_context.swap_frame_buffers();

		LOG([](auto& o) {
			o << "window wrapper constructed" << std::endl;
		})
	}

	ruis::real get_dots_per_inch()
	{
		// TODO:
		return 96; // NOLINT

		// int src_num = 0;

		// constexpr auto mm_per_cm = 10;

		// ruis::real value =
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	((ruis::real(DisplayWidth(this->display.display, src_num))
		// 	  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	  / (ruis::real(DisplayWidthMM(this->display.display, src_num)) / ruis::real(mm_per_cm)))
		// 	 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	 +
		// 	 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	 (ruis::real(DisplayHeight(this->display.display, src_num))
		// 	  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	  / (ruis::real(DisplayHeightMM(this->display.display, src_num)) / ruis::real(mm_per_cm)))) /
		// 	2;
		// constexpr auto cm_per_inch = 2.54;
		// value *= ruis::real(cm_per_inch);
		// return value;
	}

	ruis::real get_dots_per_pp()
	{
		// TODO:
		return 1;

		// // TODO: use scale factor only for desktop monitors
		// if (this->scale_factor != ruis::real(1)) {
		// 	return this->scale_factor;
		// }

		// int src_num = 0;
		// r4::vector2<unsigned> resolution(
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayWidth(this->display.display, src_num),
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayHeight(this->display.display, src_num)
		// );
		// r4::vector2<unsigned> screen_size_mm(
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayWidthMM(this->display.display, src_num),
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayHeightMM(this->display.display, src_num)
		// );

		// return application::get_pixels_per_pp(resolution, screen_size_mm);
	}
};

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
{
	ASSERT(dynamic_cast<window_wrapper*>(pimpl.get()))
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
	return static_cast<window_wrapper&>(*pimpl);
}

window_wrapper& get_impl(application& app)
{
	return get_impl(get_window_pimpl(app));
}

} // namespace

application::application(std::string name, const window_params& wp) :
	name(std::move(name)),
	window_pimpl(std::make_unique<window_wrapper>(wp)),
	gui(utki::make_shared<ruis::context>(
#ifdef RUISAPP_RENDER_OPENGL
		utki::make_shared<ruis::render_opengl::renderer>(),
#elif defined(RUISAPP_RENDER_OPENGLES)
		utki::make_shared<ruis::render_opengles::renderer>(),
#else
#	error "Unknown graphics API"
#endif
		utki::make_shared<ruis::updater>(),
		[this](std::function<void()> proc) {
			get_impl(*this).ui_queue.push_back(std::move(proc));
		},
		[](ruis::mouse_cursor c) {
			// TODO:
			// auto& ww = get_impl(*this);
			// ww.set_cursor(c);
		},
		get_impl(this->window_pimpl).get_dots_per_inch(),
		get_impl(this->window_pimpl).get_dots_per_pp()
	)),
	storage_dir(initialize_storage_dir(this->name))
{
	this->update_window_rect(ruis::rect(0, 0, ruis::real(wp.dims.x()), ruis::real(wp.dims.y())));
}

void application::swap_frame_buffers()
{
	auto& ww = get_impl(this->window_pimpl);
	ww.egl_context.swap_frame_buffers();
}

void application::set_mouse_cursor_visible(bool visible)
{
	// TODO:
}

void application::set_fullscreen(bool fullscreen)
{
	// TODO:
}

void ruisapp::application::quit() noexcept
{
	auto& ww = get_impl(this->window_pimpl);
	ww.quit_flag.store(true);
}

int main(int argc, const char** argv)
{
	std::unique_ptr<ruisapp::application> application = create_app_unix(argc, argv);
	if (!application) {
		return 1;
	}

	auto& app = *application;

	auto& ww = get_impl(app);

	opros::wait_set wait_set(2);
	wait_set.add(ww.waitable, {opros::ready::read}, &ww.waitable);
	wait_set.add(ww.ui_queue, {opros::ready::read}, &ww.ui_queue);

	utki::scope_exit scope_exit_wait_set([&]() {
		wait_set.remove(ww.ui_queue);
		wait_set.remove(ww.waitable);
	});

	while (!ww.quit_flag.load()) {
		// std::cout << "loop" << std::endl;

		while (wl_display_prepare_read(ww.display.disp) != 0) {
			// there are events in wayland queue
			if (wl_display_dispatch_pending(ww.display.disp) < 0) {
				throw std::runtime_error(utki::cat("wl_display_dispatch_pending() failed: ", strerror(errno)));
			}
		}

		{
			utki::scope_exit scope_exit_wayland_prepare_read([&]() {
				wl_display_cancel_read(ww.display.disp);
			});

			// send queued wayland requests to server
			if (wl_display_flush(ww.display.disp) < 0) {
				if (errno == EAGAIN) {
					// std::cout << "wayland display more to flush" << std::endl;
					wait_set.change(ww.waitable, {opros::ready::read, opros::ready::write}, &ww.waitable);
				} else {
					throw std::runtime_error(utki::cat("wl_display_flush() failed: ", strerror(errno)));
				}
			} else {
				// std::cout << "wayland display flushed" << std::endl;
				wait_set.change(ww.waitable, {opros::ready::read}, &ww.waitable);
			}

			wait_set.wait(app.gui.update());

			// std::cout << "waited" << std::endl;

			auto triggered_events = wait_set.get_triggered();

			// std::cout << "num triggered = " << triggered_events.size() << std::endl;

			// we want to first handle messages of ui queue,
			// but since we don't know the order of triggered objects,
			// first go through all of them and set readiness flags
			bool ui_queue_ready_to_read = false;
			bool wayland_queue_ready_to_read = false;

			for (auto& ei : triggered_events) {
				if (ei.user_data == &ww.ui_queue) {
					if (ei.flags.get(opros::ready::error)) {
						throw std::runtime_error("waiting on ui queue errored");
					}
					if (ei.flags.get(opros::ready::read)) {
						// std::cout << "ui queue ready" << std::endl;
						ui_queue_ready_to_read = true;
					}
				} else if (ei.user_data == &ww.waitable) {
					if (ei.flags.get(opros::ready::error)) {
						throw std::runtime_error("waiting on wayland file descriptor errored");
					}
					if (ei.flags.get(opros::ready::read)) {
						// std::cout << "wayland queue ready to read" << std::endl;
						wayland_queue_ready_to_read = true;
					}
				}
			}

			if (ui_queue_ready_to_read) {
				while (auto m = ww.ui_queue.pop_front()) {
					std::cout << "loop proc" << std::endl;
					m();
				}
			}

			if (wayland_queue_ready_to_read) {
				scope_exit_wayland_prepare_read.release();

				// std::cout << "read" << std::endl;
				if (wl_display_read_events(ww.display.disp) < 0) {
					throw std::runtime_error(utki::cat("wl_display_read_events() failed: ", strerror(errno)));
				}

				// std::cout << "disppatch" << std::endl;
				if (wl_display_dispatch_pending(ww.display.disp) < 0) {
					throw std::runtime_error(utki::cat("wl_display_dispatch_pending() failed: ", strerror(errno)));
				}
			}
		}
		render(app);
	}

	return 0;
}
