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
