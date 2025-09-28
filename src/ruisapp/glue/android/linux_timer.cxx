#include "linux_timer.hxx"

#include <csignal>
#include <cstring>
#include <stdexcept>

namespace {
std::function<void()> linux_timer::on_expire;
} // namespace

// Handler for SIGALRM signal
namespace {
void linux_timer::on_sigalrm(int)
{
	if (linux_timer::on_expire) {
		linux_timer::on_expire();
	}
}
} // namespace

linux_timer::linux_timer(std::function<void()> on_expire)
{
	// only one timer is allowed
	utki::assert(!linux_timer::on_expire, SL);

	linux_timer::on_expire = std::move(on_expire);

	int res = timer_create(
		CLOCK_MONOTONIC,
		0, // means SIGALRM signal is emitted when timer expires
		&this->timer
	);
	if (res != 0) {
		throw std::runtime_error("timer_create() failed");
	}

	struct sigaction sa;
	sa.sa_handler = &linux_timer::on_sigalrm;
	sa.sa_flags = SA_NODEFER;
	memset(
		&sa.sa_mask, //
		0,
		sizeof(sa.sa_mask)
	);

	res = sigaction(
		SIGALRM, //
		&sa,
		0
	);
	utki::assert(res == 0, SL);
}

linux_timer::~linux_timer()
{
	// set default handler for SIGALRM
	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	memset(
		&sa.sa_mask, //
		0,
		sizeof(sa.sa_mask)
	);

	int res = sigaction(
		SIGALRM, //
		&sa,
		0
	);
	utki::assert(
		res == 0,
		[&](auto& o) {
			o << " res = " << res << " errno = " << errno;
		},
		SL
	);

	res = timer_delete(this->timer);
	utki::assert(
		res == 0,
		[&](auto& o) {
			o << " res = " << res << " errno = " << errno;
		},
		SL
	);
}

void linux_timer::arm(uint32_t dt)
{
	itimerspec ts;
	ts.it_value.tv_sec = dt / 1000;
	ts.it_value.tv_nsec = (dt % 1000) * 1000000;
	ts.it_interval.tv_nsec = 0; // one shot timer
	ts.it_interval.tv_sec = 0; // one shot timer

	int res = timer_settime(
		this->timer, //
		0,
		&ts,
		0
	);
	utki::assert(
		res == 0,
		[&](auto& o) {
			o << " res = " << res << " errno = " << errno;
		},
		SL
	);
}

bool linux_timer::disarm()
{
	itimerspec oldts;
	itimerspec newts;
	newts.it_value.tv_nsec = 0;
	newts.it_value.tv_sec = 0;

	int res = timer_settime(
		this->timer, //
		0,
		&newts,
		&oldts
	);
	utki::assert(
		res == 0,
		[&](auto& o) {
			o << "errno = " << errno << " res = " << res;
		},
		SL
	);

	if (oldts.it_value.tv_nsec != 0 || oldts.it_value.tv_sec != 0) {
		return true;
	}
	return false;
}
