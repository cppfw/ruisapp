#pragma once

#include <array>
#include <cstdint>
#include <limits>

#include <ruis/util/key.hpp>

namespace {
// TODO: this mapping is not final
const std::array<ruis::key, size_t(std::numeric_limits<uint8_t>::max()) + 1> key_code_map = {
	ruis::key::unknown, // AKEYCODE_UNKNOWN
	ruis::key::arrow_left, // AKEYCODE_SOFT_LEFT
	ruis::key::arrow_right, // AKEYCODE_SOFT_RIGHT
	ruis::key::home, // AKEYCODE_HOME
	ruis::key::escape, // AKEYCODE_BACK
	ruis::key::f11, // AKEYCODE_CALL
	ruis::key::f12, // AKEYCODE_ENDCALL
	ruis::key::zero, // AKEYCODE_0
	ruis::key::one, // AKEYCODE_1
	ruis::key::two, // AKEYCODE_2
	ruis::key::three, // AKEYCODE_3
	ruis::key::four, // AKEYCODE_4
	ruis::key::five, // AKEYCODE_5
	ruis::key::six, // AKEYCODE_6
	ruis::key::seven, // AKEYCODE_7
	ruis::key::eight, // AKEYCODE_8
	ruis::key::nine, // AKEYCODE_9
	ruis::key::unknown, // AKEYCODE_STAR
	ruis::key::unknown, // AKEYCODE_POUND
	ruis::key::arrow_up, // AKEYCODE_DPAD_UP
	ruis::key::arrow_down, // AKEYCODE_DPAD_DOWN
	ruis::key::arrow_left, // AKEYCODE_DPAD_LEFT
	ruis::key::arrow_right, // AKEYCODE_DPAD_RIGHT
	ruis::key::enter, // AKEYCODE_DPAD_CENTER
	ruis::key::page_up, // AKEYCODE_VOLUME_UP
	ruis::key::page_down, // AKEYCODE_VOLUME_DOWN
	ruis::key::f10, // AKEYCODE_POWER
	ruis::key::f9, // AKEYCODE_CAMERA
	ruis::key::backspace, // AKEYCODE_CLEAR
	ruis::key::a, // AKEYCODE_A
	ruis::key::b, // AKEYCODE_B
	ruis::key::c, // AKEYCODE_C
	ruis::key::d, // AKEYCODE_D
	ruis::key::e, // AKEYCODE_E
	ruis::key::f, // AKEYCODE_F
	ruis::key::g, // AKEYCODE_G
	ruis::key::h, // AKEYCODE_H
	ruis::key::i, // AKEYCODE_I
	ruis::key::g, // AKEYCODE_J
	ruis::key::k, // AKEYCODE_K
	ruis::key::l, // AKEYCODE_L
	ruis::key::m, // AKEYCODE_M
	ruis::key::n, // AKEYCODE_N
	ruis::key::o, // AKEYCODE_O
	ruis::key::p, // AKEYCODE_P
	ruis::key::q, // AKEYCODE_Q
	ruis::key::r, // AKEYCODE_R
	ruis::key::s, // AKEYCODE_S
	ruis::key::t, // AKEYCODE_T
	ruis::key::u, // AKEYCODE_U
	ruis::key::v, // AKEYCODE_V
	ruis::key::w, // AKEYCODE_W
	ruis::key::x, // AKEYCODE_X
	ruis::key::y, // AKEYCODE_Y
	ruis::key::z, // AKEYCODE_Z
	ruis::key::v, // AKEYCODE_COMMA
	ruis::key::b, // AKEYCODE_PERIOD
	ruis::key::n, // AKEYCODE_ALT_LEFT
	ruis::key::m, // AKEYCODE_ALT_RIGHT
	ruis::key::left_shift, // AKEYCODE_SHIFT_LEFT
	ruis::key::right_shift, // AKEYCODE_SHIFT_RIGHT
	ruis::key::tabulator, // AKEYCODE_TAB
	ruis::key::space, // AKEYCODE_SPACE
	ruis::key::left_control, // AKEYCODE_SYM
	ruis::key::f8, // AKEYCODE_EXPLORER
	ruis::key::f7, // AKEYCODE_ENVELOPE
	ruis::key::enter, // AKEYCODE_ENTER
	ruis::key::deletion, // AKEYCODE_DEL
	ruis::key::f6, // AKEYCODE_GRAVE
	ruis::key::minus, // AKEYCODE_MINUS
	ruis::key::equals, // AKEYCODE_EQUALS
	ruis::key::left_square_bracket, // AKEYCODE_LEFT_BRACKET
	ruis::key::right_square_bracket, // AKEYCODE_RIGHT_BRACKET
	ruis::key::backslash, // AKEYCODE_BACKSLASH
	ruis::key::semicolon, // AKEYCODE_SEMICOLON
	ruis::key::apostrophe, // AKEYCODE_APOSTROPHE
	ruis::key::slash, // AKEYCODE_SLASH
	ruis::key::grave, // AKEYCODE_AT
	ruis::key::f5, // AKEYCODE_NUM
	ruis::key::f4, // AKEYCODE_HEADSETHOOK
	ruis::key::f3, // AKEYCODE_FOCUS (camera focus)
	ruis::key::f2, // AKEYCODE_PLUS
	ruis::key::f1, // AKEYCODE_MENU
	ruis::key::end, // AKEYCODE_NOTIFICATION
	ruis::key::right_control, // AKEYCODE_SEARCH
	ruis::key::unknown, // AKEYCODE_MEDIA_PLAY_PAUSE
	ruis::key::unknown, // AKEYCODE_MEDIA_STOP
	ruis::key::unknown, // AKEYCODE_MEDIA_NEXT
	ruis::key::unknown, // AKEYCODE_MEDIA_PREVIOUS
	ruis::key::unknown, // AKEYCODE_MEDIA_REWIND
	ruis::key::unknown, // AKEYCODE_MEDIA_FAST_FORWARD
	ruis::key::unknown, // AKEYCODE_MUTE
	ruis::key::page_up, // AKEYCODE_PAGE_UP
	ruis::key::page_down, // AKEYCODE_PAGE_DOWN
	ruis::key::unknown, // AKEYCODE_PICTSYMBOLS
	ruis::key::capslock, // AKEYCODE_SWITCH_CHARSET
	ruis::key::unknown, // AKEYCODE_BUTTON_A
	ruis::key::unknown, // AKEYCODE_BUTTON_B
	ruis::key::unknown, // AKEYCODE_BUTTON_C
	ruis::key::unknown, // AKEYCODE_BUTTON_X
	ruis::key::unknown, // AKEYCODE_BUTTON_Y
	ruis::key::unknown, // AKEYCODE_BUTTON_Z
	ruis::key::unknown, // AKEYCODE_BUTTON_L1
	ruis::key::unknown, // AKEYCODE_BUTTON_R1
	ruis::key::unknown, // AKEYCODE_BUTTON_L2
	ruis::key::unknown, // AKEYCODE_BUTTON_R2
	ruis::key::unknown, // AKEYCODE_BUTTON_THUMBL
	ruis::key::unknown, // AKEYCODE_BUTTON_THUMBR
	ruis::key::unknown, // AKEYCODE_BUTTON_START
	ruis::key::unknown, // AKEYCODE_BUTTON_SELECT
	ruis::key::unknown, // AKEYCODE_BUTTON_MODE
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown, //
	ruis::key::unknown //
};
} // namespace
