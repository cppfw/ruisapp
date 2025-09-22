#pragma once

#include <array>
#include <limits>

#include <utki/config.hpp>

#if CFG_COMPILER == CFG_COMPILER_MSVC
#	include <SDL.h>
#else
#	include <SDL2/SDL.h>
#endif

#include <utki/type_traits.hpp>
#include <ruis/util/key.hpp>

namespace {
const std::array<ruis::key, size_t(std::numeric_limits<uint8_t>::max()) + 1> key_code_map = {
	{
     ruis::key::unknown, // 0
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::a,
     ruis::key::b, // x5
		ruis::key::c,
     ruis::key::d,
     ruis::key::e,
     ruis::key::f,
     ruis::key::g, // 10
		ruis::key::h,
     ruis::key::i,
     ruis::key::j,
     ruis::key::k,
     ruis::key::l, // x5
		ruis::key::m,
     ruis::key::n,
     ruis::key::o,
     ruis::key::p,
     ruis::key::q, // 20
		ruis::key::r,
     ruis::key::s,
     ruis::key::t,
     ruis::key::u,
     ruis::key::v, // x5
		ruis::key::w,
     ruis::key::x,
     ruis::key::y,
     ruis::key::z,
     ruis::key::one, // 30
		ruis::key::two,
     ruis::key::three,
     ruis::key::four,
     ruis::key::five,
     ruis::key::six, // x5
		ruis::key::seven,
     ruis::key::eight,
     ruis::key::nine,
     ruis::key::zero,
     ruis::key::enter, // 40
		ruis::key::escape,
     ruis::key::backspace,
     ruis::key::tabulator,
     ruis::key::space,
     ruis::key::minus, // x5
		ruis::key::equals,
     ruis::key::left_square_bracket,
     ruis::key::right_square_bracket,
     ruis::key::backslash,
     ruis::key::backslash, // 50
		ruis::key::semicolon,
     ruis::key::apostrophe,
     ruis::key::grave,
     ruis::key::comma,
     ruis::key::period, // x5
		ruis::key::slash,
     ruis::key::capslock,
     ruis::key::f1,
     ruis::key::f2,
     ruis::key::f3, // 60
		ruis::key::f4,
     ruis::key::f5,
     ruis::key::f6,
     ruis::key::f7,
     ruis::key::f8, // x5
		ruis::key::f9,
     ruis::key::f10,
     ruis::key::f11,
     ruis::key::f12,
     ruis::key::print_screen, // 70
		ruis::key::unknown,
     ruis::key::pause,
     ruis::key::insert,
     ruis::key::home,
     ruis::key::page_up, // x5
		ruis::key::deletion,
     ruis::key::end,
     ruis::key::page_down,
     ruis::key::arrow_right,
     ruis::key::arrow_left, // 80
		ruis::key::arrow_down,
     ruis::key::arrow_up,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 90
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 100
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::f13,
     ruis::key::f14, // x5
		ruis::key::f15,
     ruis::key::f16,
     ruis::key::f17,
     ruis::key::f18,
     ruis::key::f19, // 110
		ruis::key::f20,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 120
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 130
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 140
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 150
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 160
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 170
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 180
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 190
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 200
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 210
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 220
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::left_control,
     ruis::key::left_shift, // x5
		ruis::key::left_alt,
     ruis::key::unknown,
     ruis::key::right_control,
     ruis::key::right_shift,
     ruis::key::right_alt, // 230
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 240
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // x5
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown, // 250
		ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown,
     ruis::key::unknown // 255
	}
};

inline ruis::key sdl_scancode_to_ruis_key(SDL_Scancode sc)
{
	if (size_t(sc) >= key_code_map.size()) {
		return ruis::key::unknown;
	}

	return key_code_map[sc];
}
} // namespace
