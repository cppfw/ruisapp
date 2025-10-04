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

#include <ctime>

#include <utki/debug.hpp>

namespace {
class linux_timer
{
	timer_t timer;

	static std::function<void()> on_expire;

	static void on_sigalrm(int);

public:
	linux_timer(std::function<void()> on_expire);

	~linux_timer();

	// if timer is already armed, it will re-set the expiration time
	void arm(uint32_t dt);

	// returns true if timer was disarmed
	// returns false if timer has fired before it was disarmed.
	bool disarm();
};
} // namespace
