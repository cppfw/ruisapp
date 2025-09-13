#pragma once

#include <array>
#include <limits>

#include <ruis/util/key.hpp>

namespace {
const std::array<ruis::key, unsigned(std::numeric_limits<uint8_t>::max()) + 1> key_code_map = {
	{
     ruis::key::a, // 0
		ruis::key::s,
     ruis::key::d,
     ruis::key::f,
     ruis::key::h,
     ruis::key::g, // 5
		ruis::key::z,
     ruis::key::x,
     ruis::key::c,
     ruis::key::v,
     ruis::key::unknown, // 0x0A
		ruis::key::b,
     ruis::key::q,
     ruis::key::w,
     ruis::key::e,
     ruis::key::r, // 15
		ruis::key::y,
     ruis::key::t,
     ruis::key::one,
     ruis::key::two,
     ruis::key::three, // 20
		ruis::key::four,
     ruis::key::six,
     ruis::key::five, // 0x17
		ruis::key::equals,
     ruis::key::nine, // 25
		ruis::key::seven,
     ruis::key::minus,
     ruis::key::eight,
     ruis::key::zero,
     ruis::key::right_square_bracket, // 30
		ruis::key::o,
     ruis::key::u,
     ruis::key::left_square_bracket,
     ruis::key::i,
     ruis::key::p, // 35
		ruis::key::enter, // 0x24
		ruis::key::l,
     ruis::key::j,
     ruis::key::apostrophe,
     ruis::key::k, // 40
		ruis::key::semicolon,
     ruis::key::backslash,
     ruis::key::comma,
     ruis::key::slash,
     ruis::key::n, // 0x2D, 45
		ruis::key::m,
     ruis::key::period,
     ruis::key::tabulator, // 0x30
		ruis::key::space, // 0x31
		ruis::key::grave, // 50
		ruis::key::backspace, // 0x33
		ruis::key::unknown, // 0x34
		ruis::key::escape, // 0x35
		ruis::key::unknown, // 0x36
		ruis::key::left_command, // Command, 0x37, 55
		ruis::key::left_shift, // 0x38
		ruis::key::capslock, // 0x39
		ruis::key::unknown, // Option, 0x3A
		ruis::key::left_control, // 0x3B
		ruis::key::right_shift, // 0x3C, 60
		ruis::key::unknown, // RightOption, 0x3D
		ruis::key::right_control, // 0x3E
		ruis::key::function, // 0x3F
		ruis::key::f17, // 0x40
		ruis::key::unknown, // KeypadDecimal, 0x41, 65
		ruis::key::unknown, // 0x42
		ruis::key::unknown, // KeypadMultiplym 0x43
		ruis::key::unknown, // 0x44
		ruis::key::unknown, // KeypadPlus, 0x45
		ruis::key::unknown, // 0x46, 70
		ruis::key::unknown, // KeypadClear, 0x47
		ruis::key::unknown, // VolumeUp, 0x48
		ruis::key::unknown, // VolumeDown, 0x49
		ruis::key::unknown, // Mute, 0x4A
		ruis::key::unknown, // KeypadDivide, 0x4B, 75
		ruis::key::unknown, // KeypadEnter, 0x4C
		ruis::key::unknown, // 0x4D
		ruis::key::unknown, // KeypadMinus
		ruis::key::f18, // 0x4F
		ruis::key::f19, // 0x50, 80
		ruis::key::unknown, // KeypadEquals, 0x51
		ruis::key::unknown, // Keypad0
		ruis::key::unknown, // Keypad1
		ruis::key::unknown, // Keypad2
		ruis::key::unknown, // Keypad3, 85
		ruis::key::unknown, // Keypad4
		ruis::key::unknown, // Keypad5
		ruis::key::unknown, // Keypad6
		ruis::key::unknown, // Keypad7, 0x59
		ruis::key::f20, // 0x5A, 90
		ruis::key::unknown, // Keypad8, 0x5B
		ruis::key::unknown, // Keypad9, 0x5C
		ruis::key::unknown, // 0x5D
		ruis::key::unknown, // 0x5E
		ruis::key::unknown, // 0x5F, 95
		ruis::key::f5, // 0x60
		ruis::key::f6, // 0x61
		ruis::key::f7, // 0x62
		ruis::key::f3, // 0x63
		ruis::key::f8, // 0x64, 100
		ruis::key::f9, // 0x65
		ruis::key::unknown, // 0x66
		ruis::key::f11, // 0x67
		ruis::key::unknown, // 0x68
		ruis::key::f13, // 0x69
		ruis::key::f16, // 0x6A
		ruis::key::f14, // 0x6B
		ruis::key::unknown, // 0x6C
		ruis::key::f10, // 0x6D
		ruis::key::unknown, // 0x6E
		ruis::key::f12, // 0x6F
		ruis::key::unknown, // 0x70
		ruis::key::f15, // 0x71
		ruis::key::unknown, // Help, 0x72
		ruis::key::home, // 0x73
		ruis::key::page_up, // 0x74
		ruis::key::deletion, // 0x75
		ruis::key::f4, // 0x76
		ruis::key::end, // 0x77
		ruis::key::f2, // 0x78
		ruis::key::page_down, // 0x79
		ruis::key::f1, // 0x7A
		ruis::key::arrow_left, // 0x7B
		ruis::key::arrow_right, // 0x7C
		ruis::key::arrow_down, // 0x7D
		ruis::key::arrow_up, // 0x7E
		ruis::key::unknown, // 0x7F
		ruis::key::unknown, // 0x80
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
     ruis::key::unknown, // 0x90
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
     ruis::key::unknown, // 0xA0
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
     ruis::key::unknown, // 0xB0
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
     ruis::key::unknown, // 0xC0
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
     ruis::key::unknown, // 0xD0
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
     ruis::key::unknown, // 0xE0
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
     ruis::key::unknown, // 0xF0
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
     ruis::key::unknown // 0xFF
	}
};
} // namespace
