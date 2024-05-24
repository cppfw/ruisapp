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
#include <optional>

#include <nitki/queue.hpp>
#include <opros/wait_set.hpp>
#include <papki/fs_file.hpp>
#include <sys/mman.h>
#include <utki/destructable.hpp>
#include <utki/unicode.hpp>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers
#include <xkbcommon/xkbcommon.h>

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
struct registry_wrapper;
} // namespace

namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool application_constructed = false;
} // namespace

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

namespace {
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
};
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
class keyboard_wrapper
{
	unsigned num_connected = 0;

	wl_keyboard* keyboard = nullptr;

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

		void set(xkb_state* state, xkb_keymap* keymap) noexcept
		{
			xkb_state_unref(this->state);
			xkb_keymap_unref(this->keymap);
			this->state = state;
			this->keymap = keymap;
		}
	} xkb;

	static void wl_keyboard_keymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
	{
		ASSERT(data)
		auto& self = *static_cast<keyboard_wrapper*>(data);

		// std::cout << "keymap" << std::endl;

		ASSERT(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)

		auto map_shm = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, "using C API")
		if (map_shm == MAP_FAILED) {
			throw std::runtime_error("could not map shared memory to read wayland keymap");
		}

		utki::scope_exit scope_exit_mmap([&]() {
			munmap(map_shm, size);
			close(fd);
		});

		xkb_keymap* keymap = xkb_keymap_new_from_string(
			self.xkb.context,
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
		void* data,
		wl_keyboard* keyboard,
		uint32_t serial,
		wl_surface* surface,
		wl_array* keys
	)
	{
		LOG([](auto& o) {
			o << "keyboard enter" << std::endl;
		})

		// notify ruis about pressed keys
		ASSERT(keys)
		ASSERT(keys->size % sizeof(uint32_t) == 0)
		for (auto key : utki::make_span(static_cast<uint32_t*>(keys->data), keys->size / sizeof(uint32_t))) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
			ruis::key ruis_key = key_code_map[std::uint8_t(key)];
			handle_key_event(ruisapp::inst(), true, ruis_key);
		}
	}

	static void wl_keyboard_leave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
	{
		LOG([](auto& o) {
			o << "keyboard leave" << std::endl;
		})
		// TODO: send key releases
	}

	static void wl_keyboard_key(
		void* data,
		wl_keyboard* keyboard,
		uint32_t serial,
		uint32_t time,
		uint32_t key,
		uint32_t state
	)
	{
		ASSERT(data)
		auto& self = *static_cast<keyboard_wrapper*>(data);

		// std::cout << "keyboard key = " << key << ", pressed = " << (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		// 		  << std::endl;

		bool is_pressed = state == WL_KEYBOARD_KEY_STATE_PRESSED;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
		ruis::key ruis_key = key_code_map[std::uint8_t(key)];
		handle_key_event(ruisapp::inst(), is_pressed, ruis_key);

		class unicode_provider : public ruis::gui::input_string_provider
		{
			uint32_t key;
			xkb_wrapper& xkb;

		public:
			unicode_provider(uint32_t key, xkb_wrapper& xkb) :
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

				xkb_state_key_get_utf8(this->xkb.state, keycode, buf.data(), buf.size() - 1);

				buf.back() = '\0';

				return utki::to_utf32(buf.data());
			}
		};

		if (is_pressed) {
			handle_character_input( //
				ruisapp::inst(),
				unicode_provider(key, self.xkb),
				ruis_key
			);
		}
	}

	static void wl_keyboard_modifiers(
		void* data,
		wl_keyboard* keyboard,
		uint32_t serial,
		uint32_t mods_depressed,
		uint32_t mods_latched,
		uint32_t mods_locked,
		uint32_t group
	)
	{
		ASSERT(data)
		auto& self = *static_cast<keyboard_wrapper*>(data);

		// std::cout << "modifiers" << std::endl;

		xkb_state_update_mask(self.xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
	}

	static void wl_keyboard_repeat_info(void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay)
	{
		LOG([](auto& o) {
			o << "repeat info" << std::endl;
		})
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

	keyboard_wrapper(const keyboard_wrapper&) = delete;
	keyboard_wrapper& operator=(const keyboard_wrapper&) = delete;

	keyboard_wrapper(keyboard_wrapper&&) = delete;
	keyboard_wrapper& operator=(keyboard_wrapper&&) = delete;

	~keyboard_wrapper()
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

namespace {
class output_wrapper
{
	wl_output* output;

	static void wl_output_geometry(
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
		ASSERT(data)
		auto& self = *static_cast<output_wrapper*>(data);

		self.physical_size_mm = {ruis::real(physical_width), ruis::real(physical_height)};

		LOG([&](auto& o) {
			o << "output(" << self.id << ")" << '\n' //
			  << "  physical_size_mm = " << self.physical_size_mm << '\n' //
			  << "  make = " << make << '\n' //
			  << "  model = " << model << std::endl;
		})
	}

	static void wl_output_mode(
		void* data,
		struct wl_output* wl_output,
		uint32_t flags,
		int32_t width,
		int32_t height,
		int32_t refresh
	)
	{
		ASSERT(data)
		auto& self = *static_cast<output_wrapper*>(data);

		self.resolution = {ruis::real(width), ruis::real(height)};

		LOG([&](auto& o) {
			o << "output(" << self.id << ") resolution = " << self.resolution << std::endl;
		})
	}

	static void wl_output_done(void* data, struct wl_output* wl_output)
	{
		ASSERT(data)
#ifdef DEBUG
		auto& self = *static_cast<output_wrapper*>(data);
#endif

		LOG([&](auto& o) {
			o << "output(" << self.id << ") done" << std::endl;
		})
	}

	static void wl_output_scale(void* data, struct wl_output* wl_output, int32_t factor)
	{
		ASSERT(data)
		auto& self = *static_cast<output_wrapper*>(data);

		self.scale = ruis::real(factor);

		LOG([&](auto& o) {
			o << "output(" << self.id << ") scale = " << self.scale << std::endl;
		})
	}

	static void wl_output_name(void* data, struct wl_output* wl_output, const char* name)
	{
		ASSERT(data)
#ifdef DEBUG
		auto& self = *static_cast<output_wrapper*>(data);
#endif

		LOG([&](auto& o) {
			o << "output(" << self.id << ") name = " << name << std::endl;
		})
	}

	static void wl_output_description(void* data, struct wl_output* wl_output, const char* description)
	{
		ASSERT(data)
#ifdef DEBUG
		auto& self = *static_cast<output_wrapper*>(data);
#endif

		LOG([&](auto& o) {
			o << "output(" << self.id << ") description = " << description << std::endl;
		})
	}

	constexpr static const wl_output_listener listener = {
		.geometry = &wl_output_geometry,
		.mode = &wl_output_mode,
		.done = &wl_output_done,
		.scale = &wl_output_scale,

		// TODO: wayland version included in debian bullseye does not support these fields,
		//       uncomment them when debian bullseye support can be dropped
		// .name = &wl_output_name,
		// .description = &wl_output_description
	};

public:
	const uint32_t id;

	ruis::vec2 resolution = {0, 0};
	ruis::vec2 physical_size_mm = {0, 0};
	ruis::real scale = 1;

	output_wrapper(wl_registry& registry, uint32_t id) :
		output([&]() {
			void* output = wl_registry_bind(&registry, id, &wl_output_interface, 1);
			ASSERT(output)
			return static_cast<wl_output*>(output);
		}()),
		id(id)
	{
		wl_output_add_listener(this->output, &listener, this);
	}

	output_wrapper(const output_wrapper&) = delete;
	output_wrapper& operator=(const output_wrapper&) = delete;

	output_wrapper(output_wrapper&&) = delete;
	output_wrapper& operator=(output_wrapper&&) = delete;

	~output_wrapper()
	{
		if (wl_output_get_version(this->output) >= WL_OUTPUT_RELEASE_SINCE_VERSION) {
			wl_output_release(this->output);
		} else {
			wl_output_destroy(this->output);
		}
	}
};
} // namespace

namespace {
struct registry_wrapper {
	wl_registry* reg;

	std::optional<uint32_t> compositor_id;
	std::optional<uint32_t> wm_base_id;
	std::optional<uint32_t> shm_id;
	std::optional<uint32_t> seat_id;

	std::list<output_wrapper> outputs;

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
		if (std::string_view(interface) == "wl_compositor"sv) {
			self.compositor_id = id;
		} else if (std::string_view(interface) == xdg_wm_base_interface.name) {
			self.wm_base_id = id;
		} else if (std::string_view(interface) == "wl_seat"sv) {
			self.seat_id = id;
		} else if (std::string_view(interface) == "wl_shm"sv) {
			self.shm_id = id;
		} else if (std::string_view(interface) == "wl_output"sv) {
			ASSERT(self.reg)
			self.outputs.emplace_back(*self.reg, id);
		}

		// std::cout << "exit from registry event" << std::endl;
	}

	static void wl_registry_global_remove(void* data, wl_registry* registry, uint32_t id)
	{
		LOG([&](auto& o) {
			o << "got a registry losing event, id = " << id << std::endl;
		});

		ASSERT(data)
		auto& self = *static_cast<registry_wrapper*>(data);

		// check if removed object is a wl_output
		for (auto i = self.outputs.begin(); i != self.outputs.end(); ++i) {
			if (i->id == id) {
				LOG([&](auto& o) {
					o << "output removed, id = " << id << std::endl;
				});
				self.outputs.erase(i);
				return;
			}
		}
	}

	constexpr static const wl_registry_listener listener = {
		.global = &wl_registry_global,
		.global_remove = wl_registry_global_remove
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
		wl_display_dispatch_pending(display.disp);

		// std::cout << "registry events dispatched" << std::endl;

		// at this point we should have needed object ids set by global registry handler

		if (!this->compositor_id.has_value()) {
			throw std::runtime_error("could not find wayland compositor");
		}

		if (!this->wm_base_id.has_value()) {
			throw std::runtime_error("could not find xdg_shell");
		}

		if (!this->seat_id.has_value()) {
			throw std::runtime_error("could not find wl_seat");
		}

		if (!this->shm_id.has_value()) {
			throw std::runtime_error("could not find wl_shm");
		}

		registry_scope_exit.release();

		// std::cout << "registry constructed" << std::endl;
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
		wl_registry_destroy(this->reg);
	}
};
} // namespace

namespace {
struct compositor_wrapper {
	wl_compositor* const comp;

	compositor_wrapper(const registry_wrapper& registry) :
		comp([&]() {
			ASSERT(registry.compositor_id.has_value())
			void* compositor =
				wl_registry_bind(registry.reg, registry.compositor_id.value(), &wl_compositor_interface, 1);
			ASSERT(compositor)
			return static_cast<wl_compositor*>(compositor);
		}())
	{}

	compositor_wrapper(const compositor_wrapper&) = delete;
	compositor_wrapper& operator=(const compositor_wrapper&) = delete;

	compositor_wrapper(compositor_wrapper&&) = delete;
	compositor_wrapper& operator=(compositor_wrapper&&) = delete;

	~compositor_wrapper()
	{
		wl_compositor_destroy(this->comp);
	}
};
} // namespace

namespace {
struct shm_wrapper {
	wl_shm* const shm;

	shm_wrapper(const registry_wrapper& registry) :
		shm([&]() {
			ASSERT(registry.shm_id.has_value())
			void* shm = wl_registry_bind(registry.reg, registry.shm_id.value(), &wl_shm_interface, 1);
			ASSERT(shm)
			return static_cast<wl_shm*>(shm);
		}())
	{}

	shm_wrapper(const shm_wrapper&) = delete;
	shm_wrapper& operator=(const shm_wrapper&) = delete;

	shm_wrapper(shm_wrapper&&) = delete;
	shm_wrapper& operator=(shm_wrapper&&) = delete;

	~shm_wrapper()
	{
		wl_shm_destroy(this->shm);
	}
};
} // namespace

namespace {
struct wm_base_wrapper {
	xdg_wm_base* const wmb;

	wm_base_wrapper(const registry_wrapper& registry) :
		wmb([&]() {
			ASSERT(registry.wm_base_id.has_value())
			void* wm_base = wl_registry_bind(registry.reg, registry.wm_base_id.value(), &xdg_wm_base_interface, 1);
			ASSERT(wm_base)
			return static_cast<xdg_wm_base*>(wm_base);
		}())
	{
		xdg_wm_base_add_listener(this->wmb, &listener, this);
	}

	wm_base_wrapper(const wm_base_wrapper&) = delete;
	wm_base_wrapper& operator=(const wm_base_wrapper&) = delete;

	wm_base_wrapper(wm_base_wrapper&&) = delete;
	wm_base_wrapper& operator=(wm_base_wrapper&&) = delete;

	~wm_base_wrapper()
	{
		xdg_wm_base_destroy(this->wmb);
	}

private:
	constexpr static const xdg_wm_base_listener listener = {
		.ping =
			[](void* data, xdg_wm_base* wm_base, uint32_t serial) {
				xdg_wm_base_pong(wm_base, serial);
			} //
	};
};
} // namespace

namespace {
struct region_wrapper {
	wl_region* reg;

	region_wrapper(compositor_wrapper& compositor) :
		reg(wl_compositor_create_region(compositor.comp))
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
} // namespace

namespace {
struct surface_wrapper {
	wl_surface* const sur;

	surface_wrapper(const compositor_wrapper& compositor) :
		sur(wl_compositor_create_surface(compositor.comp))
	{
		if (!this->sur) {
			throw std::runtime_error("could not create wayland surface");
		}

		wl_surface_add_listener(this->sur, &listener, this);
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

private:
	constexpr static const wl_surface_listener listener = {
		.enter =
			[](void* data, wl_surface* surface, wl_output* output) {
				LOG([](auto& o) {
					o << "surface enters output" << std::endl;
				})
			},
		.leave =
			[](void* data, wl_surface* surface, wl_output* output) {
				LOG([](auto& o) {
					o << "surface leaves output" << std::endl;
				})
			}
	};
};
} // namespace

namespace {
struct cursor_theme_wrapper {
	constexpr static auto cursor_size = 32;

	cursor_theme_wrapper(const shm_wrapper& shm) :
		theme(wl_cursor_theme_load(nullptr, cursor_size, shm.shm))
	{
		if (!this->theme) {
			// no default theme
			return;
		}

		this->cursors[size_t(ruis::mouse_cursor::arrow)] = wl_cursor_theme_get_cursor(this->theme, "left_ptr");
		this->cursors[size_t(ruis::mouse_cursor::top_left_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "top_left_corner");
		this->cursors[size_t(ruis::mouse_cursor::top_right_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "top_right_corner");
		this->cursors[size_t(ruis::mouse_cursor::bottom_left_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "bottom_left_corner");
		this->cursors[size_t(ruis::mouse_cursor::bottom_right_corner)] =
			wl_cursor_theme_get_cursor(this->theme, "bottom_right_corner");
		this->cursors[size_t(ruis::mouse_cursor::top_side)] = wl_cursor_theme_get_cursor(this->theme, "top_side");
		this->cursors[size_t(ruis::mouse_cursor::bottom_side)] = wl_cursor_theme_get_cursor(this->theme, "bottom_side");
		this->cursors[size_t(ruis::mouse_cursor::left_side)] = wl_cursor_theme_get_cursor(this->theme, "left_side");
		this->cursors[size_t(ruis::mouse_cursor::right_side)] = wl_cursor_theme_get_cursor(this->theme, "right_side");
		this->cursors[size_t(ruis::mouse_cursor::grab)] = wl_cursor_theme_get_cursor(this->theme, "grabbing");
		this->cursors[size_t(ruis::mouse_cursor::index_finger)] = wl_cursor_theme_get_cursor(this->theme, "hand1");
		this->cursors[size_t(ruis::mouse_cursor::caret)] = wl_cursor_theme_get_cursor(this->theme, "xterm");
	}

	cursor_theme_wrapper(const cursor_theme_wrapper&) = delete;
	cursor_theme_wrapper& operator=(const cursor_theme_wrapper&) = delete;

	cursor_theme_wrapper(cursor_theme_wrapper&&) = delete;
	cursor_theme_wrapper& operator=(cursor_theme_wrapper&&) = delete;

	~cursor_theme_wrapper()
	{
		if (this->theme) {
			wl_cursor_theme_destroy(this->theme);
		}
	}

	wl_cursor* get(ruis::mouse_cursor cursor)
	{
		auto index = size_t(cursor);
		ASSERT(index < this->cursors.size())
		return this->cursors[index];
	}

private:
	wl_cursor_theme* const theme;

	std::array<wl_cursor*, size_t(ruis::mouse_cursor::enum_size)> cursors = {nullptr};
};
} // namespace

namespace {
struct pointer_wrapper {
	cursor_theme_wrapper cursor_theme;

	surface_wrapper cursor_surface;

	ruis::vector2 cur_pointer_pos{0, 0};

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

	void set_cursor()
	{
		if (this->cursor_visible) {
			this->apply_cursor(this->current_cursor);
		} else {
			wl_pointer_set_cursor(this->pointer, this->last_enter_serial, nullptr, 0, 0);
		}
	}

	void set_cursor(ruis::mouse_cursor c)
	{
		this->current_cursor = this->cursor_theme.get(c);

		this->set_cursor();
	}

	void set_cursor_visible(bool visible)
	{
		this->cursor_visible = visible;

		if (!this->pointer) {
			return;
		}

		if (visible) {
			this->apply_cursor(this->current_cursor);
		} else {
			wl_pointer_set_cursor(this->pointer, this->last_enter_serial, nullptr, 0, 0);
		}
	}

	pointer_wrapper(const compositor_wrapper& compositor, const shm_wrapper& shm) :
		cursor_theme(shm),
		cursor_surface(compositor),
		current_cursor(this->cursor_theme.get(ruis::mouse_cursor::arrow))
	{}

	pointer_wrapper(const pointer_wrapper&) = delete;
	pointer_wrapper& operator=(const pointer_wrapper&) = delete;

	pointer_wrapper(pointer_wrapper&&) = delete;
	pointer_wrapper& operator=(pointer_wrapper&&) = delete;

	~pointer_wrapper()
	{
		if (this->pointer) {
			if (wl_pointer_get_version(this->pointer) >= WL_POINTER_RELEASE_SINCE_VERSION) {
				wl_pointer_release(this->pointer);
			} else {
				wl_pointer_destroy(this->pointer);
			}
		}
	}

private:
	static void wl_pointer_enter(
		void* data,
		wl_pointer* pointer,
		uint32_t serial,
		wl_surface* surface,
		wl_fixed_t x,
		wl_fixed_t y
	) //
	{
		// std::cout << "mouse enter: x,y = " << std::dec << x << ", " << y << std::endl;
		auto& self = *static_cast<pointer_wrapper*>(data);
		self.last_enter_serial = serial;

		self.set_cursor();

		handle_mouse_hover(ruisapp::inst(), true, 0);
		self.cur_pointer_pos = ruis::vector2(wl_fixed_to_int(x), wl_fixed_to_int(y));
		handle_mouse_move(ruisapp::inst(), self.cur_pointer_pos, 0);
	}

	static void wl_pointer_motion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
	{
		// std::cout << "mouse move: x,y = " << std::dec << x << ", " << y << std::endl;
		auto& self = *static_cast<pointer_wrapper*>(data);
		self.cur_pointer_pos = ruis::vector2(wl_fixed_to_int(x), wl_fixed_to_int(y));
		handle_mouse_move(ruisapp::inst(), self.cur_pointer_pos, 0);
	}

	static void wl_pointer_button(
		void* data,
		wl_pointer* pointer,
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

	static void wl_pointer_axis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
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
			[](void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface) {
				// std::cout << "mouse leave" << std::endl;
				handle_mouse_hover(ruisapp::inst(), false, 0);
			},
		.motion = &wl_pointer_motion,
		.button = &wl_pointer_button,
		.axis = &wl_pointer_axis,
		.frame =
			[](void* data, wl_pointer* pointer) {
				LOG([](auto& o) {
					o << "pointer frame" << std::endl;
				})
			},
		.axis_source =
			[](void* data, wl_pointer* pointer, uint32_t source) {
				LOG([&](auto& o) {
					o << "axis source: " << std::dec << source << std::endl;
				})
			},
		.axis_stop =
			[](void* data, wl_pointer* pointer, uint32_t time, uint32_t axis) {
				LOG([&](auto& o) {
					o << "axis stop: axis = " << std::dec << axis << std::endl;
				})
			},
		.axis_discrete =
			[](void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete) {
				LOG([&](auto& o) {
					o << "axis discrete: axis = " << std::dec << axis << ", discrete = " << discrete << std::endl;
				})
			}
	};

	unsigned num_connected = 0;

	wl_pointer* pointer = nullptr;

	bool cursor_visible = true;

	wl_cursor* current_cursor;

	uint32_t last_enter_serial = 0;

	void apply_cursor(wl_cursor* cursor)
	{
		if (!this->pointer) {
			// no pointer connected
			return;
		}

		utki::scope_exit scope_exit_empty_cursor([this]() {
			wl_pointer_set_cursor(this->pointer, this->last_enter_serial, nullptr, 0, 0);
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
			this->pointer,
			this->last_enter_serial,
			this->cursor_surface.sur,
			int32_t(image->hotspot_x),
			int32_t(image->hotspot_y)
		);

		wl_surface_attach(this->cursor_surface.sur, buffer, 0, 0);

		wl_surface_damage(this->cursor_surface.sur, 0, 0, int32_t(image->width), int32_t(image->height));

		this->cursor_surface.commit();

		scope_exit_empty_cursor.release();
	}
};
} // namespace

namespace {
class seat_wrapper
{
public:
	pointer_wrapper pointer;
	keyboard_wrapper keyboard;

	seat_wrapper(const registry_wrapper& registry, const compositor_wrapper& compositor, const shm_wrapper& shm) :
		pointer(compositor, shm),
		seat([&]() {
			ASSERT(registry.seat_id.has_value())
			void* seat = wl_registry_bind(registry.reg, registry.seat_id.value(), &wl_seat_interface, 1);
			ASSERT(seat)
			return static_cast<wl_seat*>(seat);
		}())
	{
		// std::cout << "seat constructor" << std::endl;
		wl_seat_add_listener(this->seat, &listener, this);
	}

	seat_wrapper(const seat_wrapper&) = delete;
	seat_wrapper& operator=(const seat_wrapper&) = delete;

	seat_wrapper(seat_wrapper&&) = delete;
	seat_wrapper& operator=(seat_wrapper&&) = delete;

	~seat_wrapper()
	{
		if (wl_seat_get_version(this->seat) >= WL_SEAT_RELEASE_SINCE_VERSION) {
			wl_seat_release(this->seat);
		} else {
			wl_seat_destroy(this->seat);
		}
	}

private:
	wl_seat* const seat;

	static void wl_seat_capabilities(void* data, wl_seat* wl_seat, uint32_t capabilities)
	{
		LOG([&](auto& o) {
			o << "seat capabilities: " << std::hex << "0x" << capabilities << std::endl;
		})

		auto& self = *static_cast<seat_wrapper*>(data);

		bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

		if (have_pointer) {
			LOG([&](auto& o) {
				o << "  pointer connected " << std::endl;
			})
			self.pointer.connect(self.seat);
		} else {
			LOG([&](auto& o) {
				o << "  pointer disconnected " << std::endl;
			})
			self.pointer.disconnect();
		}

		bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

		if (have_keyboard) {
			LOG([&](auto& o) {
				o << "  keyboard connected " << std::endl;
			})
			self.keyboard.connect(self.seat);
		} else {
			LOG([&](auto& o) {
				o << "  keyboard disconnected " << std::endl;
			})
			self.keyboard.disconnect();
		}
	}

	constexpr static const wl_seat_listener listener = {
		.capabilities = &wl_seat_capabilities,
		.name =
			[](void* data, wl_seat* seat, const char* name) {
				LOG([&](auto& o) {
					o << "seat name: " << name << std::endl;
				})
			} //
	};
};
} // namespace

namespace {

struct window_wrapper;

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl);
window_wrapper& get_impl(application& app);

struct window_wrapper : public utki::destructable {
	std::atomic_bool quit_flag = false;

	nitki::queue ui_queue;

	display_wrapper display;

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

	registry_wrapper registry;

	compositor_wrapper compositor;
	shm_wrapper shm;
	wm_base_wrapper wm_base;
	seat_wrapper seat;

	surface_wrapper surface;

	struct xdg_surface_wrapper {
		xdg_surface* xdg_sur;

		constexpr static const xdg_surface_listener listener = {
			.configure =
				[](void* data, xdg_surface* xdg_surface, uint32_t serial) {
					xdg_surface_ack_configure(xdg_surface, serial);
				}, //
		};

		xdg_surface_wrapper(surface_wrapper& surface, wm_base_wrapper& wm_base) :
			xdg_sur(xdg_wm_base_get_xdg_surface(wm_base.wmb, surface.sur))
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
			xdg_toplevel* xdg_toplevel,
			int32_t width,
			int32_t height,
			wl_array* states
		)
		{
			LOG([](auto& o) {
				o << "window configure" << std::endl;
			})

			LOG([&](auto& o) {
				o << "  width = " << std::dec << width << ", height = " << height << std::endl;
			})

			bool fullscreen = false;

			LOG([&](auto& o) {
				o << "  states:" << std::endl;
			})
			ASSERT(states)
			ASSERT(states->size % sizeof(uint32_t) == 0)
			for (uint32_t s : utki::make_span(static_cast<uint32_t*>(states->data), states->size / sizeof(uint32_t))) {
				switch (s) {
					case XDG_TOPLEVEL_STATE_FULLSCREEN:
						LOG([](auto& o) {
							o << "    fullscreen" << std::endl;
						})
						fullscreen = true;
						break;
					default:
						LOG([&](auto& o) {
							o << "    " <<
								[&s]() {
									switch (s) {
										case XDG_TOPLEVEL_STATE_MAXIMIZED:
											return "maximized";
										case XDG_TOPLEVEL_STATE_RESIZING:
											return "resizing";
										case XDG_TOPLEVEL_STATE_ACTIVATED:
											return "activated";
										case XDG_TOPLEVEL_STATE_TILED_LEFT:
											return "tiled left";
										case XDG_TOPLEVEL_STATE_TILED_RIGHT:
											return "tiled right";
										case XDG_TOPLEVEL_STATE_TILED_TOP:
											return "tiled top";
										case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
											return "tiled bottom";
										default:
											return "unknown";
									}
								}()
							  << std::endl;
						})
						break;
				}
			}

			if (!application_constructed) {
				// unable to obtain window_wrapper object before application is constructed,
				// cannot do more without window_wrapper object
				LOG([](auto& o) {
					o << "  called within application constructor" << std::endl;
				})
				return;
			}

			auto& ww = get_impl(ruisapp::inst());

			if (width == 0 && height == 0) {
				if (ww.fullscreen != fullscreen) {
					if (!fullscreen) {
						// exited fullscreen mode
						ww.resize(ww.pre_fullscreen_win_dims);
					}
				}
				ww.fullscreen = fullscreen;
				return;
			}

			ASSERT(width >= 0)
			ASSERT(height >= 0)

			ww.fullscreen = fullscreen;

			ww.resize({width, height});
		}

		static void xdg_toplevel_close(void* data, xdg_toplevel* xdg_toplevel)
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
					// We cannot set bits for all OpenGL ES versions because on platforms which do not
					// support later versions the matching config will not be found by eglChooseConfig().
					// So, set bits according to requested OpenGL ES version.
					[&ver = wp.graphics_api_version]() {
						EGLint ret = EGL_OPENGL_ES2_BIT; // OpenGL ES 2 is the minimum
						if (ver.major >= 3) {
							ret |= EGL_OPENGL_ES3_BIT;
						}
						return ret;
					}(),
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
		compositor(this->registry),
		shm(this->registry),
		wm_base(this->registry),
		seat(this->registry, this->compositor, this->shm),
		surface(this->compositor),
		xdg_surface(this->surface, this->wm_base),
		toplevel(this->surface, this->xdg_surface),
		region(this->compositor),
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

	bool fullscreen = false;
	r4::vector2<int> pre_fullscreen_win_dims;

	void resize(r4::vector2<int> dims)
	{
		LOG([&](auto& o) {
			o << "resize window to " << std::dec << dims << std::endl;
		})

		wl_egl_window_resize(this->egl_window.win, dims.x(), dims.y(), 0, 0);
		this->surface.commit();

		update_window_rect(ruisapp::inst(), ruis::rect(0, {ruis::real(dims.x()), ruis::real(dims.y())}));
	}
};

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
{
	ASSERT(pimpl)
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
		utki::make_shared<ruis::render::opengl::renderer>(),
#elif defined(RUISAPP_RENDER_OPENGLES)
		utki::make_shared<ruis::render::opengles::renderer>(),
#else
#	error "Unknown graphics API"
#endif
		utki::make_shared<ruis::updater>(),
		[this](std::function<void()> proc) {
			get_impl(*this).ui_queue.push_back(std::move(proc));
		},
		[this](ruis::mouse_cursor c) {
			auto& ww = get_impl(*this);
			ww.seat.pointer.set_cursor(c);
		},
		get_impl(this->window_pimpl).get_dots_per_inch(),
		get_impl(this->window_pimpl).get_dots_per_pp()
	)),
	storage_dir(initialize_storage_dir(this->name))
{
	this->update_window_rect(ruis::rect(0, 0, ruis::real(wp.dims.x()), ruis::real(wp.dims.y())));

	application_constructed = true;
}

void application::swap_frame_buffers()
{
	auto& ww = get_impl(this->window_pimpl);
	ww.egl_context.swap_frame_buffers();
}

void application::set_mouse_cursor_visible(bool visible)
{
	auto& ww = get_impl(this->window_pimpl);
	ww.seat.pointer.set_cursor_visible(visible);
}

void application::set_fullscreen(bool fullscreen)
{
	if (fullscreen == this->is_fullscreen_v) {
		return;
	}

	this->is_fullscreen_v = fullscreen;

	auto& ww = get_impl(this->window_pimpl);

	LOG([&](auto& o) {
		o << "set_fullscreen(" << fullscreen << ")" << std::endl;
	})

	if (fullscreen) {
		wl_egl_window_get_attached_size(
			ww.egl_window.win,
			&ww.pre_fullscreen_win_dims.x(),
			&ww.pre_fullscreen_win_dims.y()
		);
		LOG([&](auto& o) {
			o << " old win dims = " << std::dec << ww.pre_fullscreen_win_dims << std::endl;
		})
		xdg_toplevel_set_fullscreen(ww.toplevel.toplev, nullptr);
	} else {
		xdg_toplevel_unset_fullscreen(ww.toplevel.toplev);
	}
}

void ruisapp::application::quit() noexcept
{
	auto& ww = get_impl(this->window_pimpl);
	ww.quit_flag.store(true);
}

// NOLINTNEXTLINE(bugprone-exception-escape, "it's what we want")
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
					LOG([](auto& o) {
						o << "loop proc" << std::endl;
					})
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
