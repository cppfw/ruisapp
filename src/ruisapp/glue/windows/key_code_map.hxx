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

#pragma once

#include <array>
#include <limits>

#include <ruis/util/key.hpp>

namespace {
const std::array<
	ruis::key, //
	std::numeric_limits<uint8_t>::max() + 1 //
	>
	key_code_map = {
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // VK_LBUTTON
		ruis::key::unknown, // VK_RBUTTON
		ruis::key::unknown, // VK_CANCEL
		ruis::key::unknown, // VK_MBUTTON
		ruis::key::unknown, // VK_XBUTTON1, 5
		ruis::key::unknown, // VK_XBUTTON2
		ruis::key::unknown, // Undefined
		ruis::key::backspace, // VK_BACK = backspace key
		ruis::key::tabulator, // VK_TAB
		ruis::key::unknown, // Reserved, 10
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // VK_CLEAR = clear key???
		ruis::key::enter, // VK_RETURN
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // Undefined, 15
		ruis::key::left_shift, // VK_SHIFT
		ruis::key::left_control, // VK_CONTROL
		ruis::key::left_alt, // VK_MENU = alt key
		ruis::key::pause, // VK_PAUSE
		ruis::key::capslock, // VK_CAPITAL = caps lock key, 20
		ruis::key::unknown, // VK_KANA, VK_HANGUEL, VK_HANGUL
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // VK_JUNJA
		ruis::key::unknown, // VK_FINAL
		ruis::key::unknown, // VK_HANJA, VK_KANJI, 25
		ruis::key::unknown, // Undefined
		ruis::key::escape, // VK_ESCAPE
		ruis::key::unknown, // VK_CONVERT
		ruis::key::unknown, // VK_NONCONVERT
		ruis::key::unknown, // VK_ACCEPT, 30
		ruis::key::unknown, // VK_MODECHANGE
		ruis::key::space, // VK_SPACE = space bar key
		ruis::key::page_up, // VK_PRIOR = page up key
		ruis::key::page_down, // VK_NEXT = page down key
		ruis::key::end, // VK_END, 35
		ruis::key::home, // VK_HOME
		ruis::key::arrow_left, // VK_LEFT
		ruis::key::arrow_up, // VK_UP
		ruis::key::arrow_right, // VK_RIGHT
		ruis::key::arrow_down, // VK_DOWN, 40
		ruis::key::unknown, // VK_SELECT
		ruis::key::unknown, // VK_PRINT
		ruis::key::unknown, // VK_EXECUTE
		ruis::key::print_screen, // VK_SNAPSHOT = print screen key
		ruis::key::insert, // VK_INSERT, 45
		ruis::key::deletion, // VK_DELETE
		ruis::key::unknown, // VK_HELP
		ruis::key::zero, // 0 key
		ruis::key::one, // 1 key
		ruis::key::two, // 2 key, 50
		ruis::key::three, // 3 key
		ruis::key::four, // 4 key
		ruis::key::five, // 5 key
		ruis::key::six, // 6 key
		ruis::key::seven, // 7 key, 55
		ruis::key::eight, // 8 key
		ruis::key::nine, // 9 key
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // Undefined, 60
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // Undefined
		ruis::key::unknown, // Undefined
		ruis::key::a, // A key, 65
		ruis::key::b, // B key
		ruis::key::c, // C key
		ruis::key::d, // D key
		ruis::key::e, // E key
		ruis::key::f, // F key, 70
		ruis::key::g, // G key
		ruis::key::h, // H key
		ruis::key::i, // I key
		ruis::key::j, // J key
		ruis::key::k, // K key, 75
		ruis::key::l, // L key
		ruis::key::m, // M key
		ruis::key::n, // N key
		ruis::key::o, // O key
		ruis::key::p, // P key, 80
		ruis::key::q, // Q key
		ruis::key::r, // R key
		ruis::key::s, // S key
		ruis::key::t, // T key
		ruis::key::u, // U key, 85
		ruis::key::v, // V key
		ruis::key::w, // W key
		ruis::key::x, // X key
		ruis::key::y, // Y key
		ruis::key::z, // Z key, 90
		ruis::key::left_command, // VK_LWIN = left windows key
		ruis::key::right_command, // VK_RWIN = right windows key
		ruis::key::unknown, // VK_APPS = applications key
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // VK_SLEEP = computer sleep key, 95
		ruis::key::zero, // VK_NUMPAD0 = numeric keypad 0 key
		ruis::key::one, // VK_NUMPAD1 = numeric keypad 1 key
		ruis::key::two, // VK_NUMPAD2 = numeric keypad 2 key
		ruis::key::three, // VK_NUMPAD3 = numeric keypad 3 key
		ruis::key::four, // VK_NUMPAD4 = numeric keypad 4 key, 100
		ruis::key::five, // VK_NUMPAD5 = numeric keypad 5 key
		ruis::key::six, // VK_NUMPAD6 = numeric keypad 6 key
		ruis::key::seven, // VK_NUMPAD7 = numeric keypad 7 key
		ruis::key::eight, // VK_NUMPAD8 = numeric keypad 8 key
		ruis::key::nine, // VK_NUMPAD9 = numeric keypad 9 key, 105
		ruis::key::unknown, // VK_MULTIPLY = multiply key
		ruis::key::unknown, // VK_ADD
		ruis::key::unknown, // VK_SEPARATOR
		ruis::key::unknown, // VK_SUBTRACT
		ruis::key::unknown, // VK_DECIMAL, 110
		ruis::key::unknown, // VK_DIVIDE
		ruis::key::f1, // VK_F1
		ruis::key::f2, // VK_F2
		ruis::key::f3, // VK_F3
		ruis::key::f4, // VK_F4, 115
		ruis::key::f5, // VK_F5
		ruis::key::f6, // VK_F6
		ruis::key::f7, // VK_F7
		ruis::key::f8, // VK_F8
		ruis::key::f9, // VK_F9, 120
		ruis::key::f10, // VK_F10
		ruis::key::f11, // VK_F11
		ruis::key::f12, // VK_F12
		ruis::key::unknown, // VK_F13
		ruis::key::unknown, // VK_F14, 125
		ruis::key::unknown, // VK_F15
		ruis::key::unknown, // VK_F16
		ruis::key::unknown, // VK_F17
		ruis::key::unknown, // VK_F18
		ruis::key::unknown, // VK_F19, 130
		ruis::key::unknown, // VK_F20
		ruis::key::unknown, // VK_F21
		ruis::key::unknown, // VK_F22
		ruis::key::unknown, // VK_F23
		ruis::key::unknown, // VK_F24, 135
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned, 140
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // VK_NUMLOCK
		ruis::key::unknown, // VK_SCROLL = scroll lock key, 145
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific, 150
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned, 155
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::left_shift, // VK_LSHIFT, 160
		ruis::key::right_shift, // VK_RSHIFT
		ruis::key::left_control, // VK_LCONTROL
		ruis::key::right_control, // VK_RCONTROL
		ruis::key::menu, // VK_LMENU = left menu key
		ruis::key::menu, // VK_RMENU, 165
		ruis::key::unknown, // VK_BROWSER_BACK
		ruis::key::unknown, // VK_BROWSER_FORWARD
		ruis::key::unknown, // VK_BROWSER_REFRESH
		ruis::key::unknown, // VK_BROWSER_STOP
		ruis::key::unknown, // VK_BROWSER_SEARCH, 170
		ruis::key::unknown, // VK_BROWSER_FAVORITES
		ruis::key::unknown, // VK_BROWSER_HOME
		ruis::key::unknown, // VK_VOLUME_MUTE
		ruis::key::unknown, // VK_VOLUME_DOWN
		ruis::key::unknown, // VK_VOLUME_UP, 175
		ruis::key::unknown, // VK_MEDIA_NEXT_TRACK
		ruis::key::unknown, // VK_MEDIA_PREV_TRACK
		ruis::key::unknown, // VK_MEDIA_STOP
		ruis::key::unknown, // VK_MEDIA_PLAY_PAUSE
		ruis::key::unknown, // VK_LAUNCH_MAIL, 180
		ruis::key::unknown, // VK_LAUNCH_MEDIA_SELECT
		ruis::key::unknown, // VK_LAUNCH_APP1
		ruis::key::unknown, // VK_LAUNCH_APP2
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved, 185
		ruis::key::unknown, // VK_OEM_1
		ruis::key::unknown, // VK_OEM_PLUS
		ruis::key::unknown, // VK_OEM_COMMA
		ruis::key::unknown, // VK_OEM_MINUS
		ruis::key::unknown, // VK_OEM_PERIOD, 190
		ruis::key::unknown, // VK_OEM_2
		ruis::key::unknown, // VK_OEM_3
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved, 195
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved, 200
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved, 205
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved, 210
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // Reserved, 215
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // VK_OEM_4
		ruis::key::unknown, // VK_OEM_5, 220
		ruis::key::unknown, // VK_OEM_6
		ruis::key::unknown, // VK_OEM_7
		ruis::key::unknown, // VK_OEM_8
		ruis::key::unknown, // Reserved
		ruis::key::unknown, // OEM specific, 225
		ruis::key::unknown, // VK_OEM_102
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // VK_PROCESSKEY
		ruis::key::unknown, // OEM specific, 230
		ruis::key::unknown, // VK_PACKET
		ruis::key::unknown, // Unassigned
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific, 235
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific, 240
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific
		ruis::key::unknown, // OEM specific, 245
		ruis::key::unknown, // VK_ATTN
		ruis::key::unknown, // VK_CRSEL
		ruis::key::unknown, // VK_EXSEL
		ruis::key::unknown, // VK_EREOF
		ruis::key::unknown, // VK_PLAY, 250
		ruis::key::unknown, // VK_ZOOM
		ruis::key::unknown, // VK_NONAME
		ruis::key::unknown, // VK_PA1
		ruis::key::unknown, // VK_OEM_CLEAR
		ruis::key::unknown
};
} // namespace
