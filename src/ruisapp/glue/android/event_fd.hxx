#pragma once

#include <cerrno>
#include <system_error>

#include <sys/eventfd.h>
#include <unistd.h>
#include <utki/debug.hpp>

namespace {
class event_fd_wrapper
{
	int fd;

public:
	event_fd_wrapper()
	{
		this->fd = eventfd(
			0, //
			EFD_NONBLOCK
		);
		if (this->fd < 0) {
			throw std::system_error(
				errno, //
				std::generic_category(),
				"could not create eventFD (*nix)"
			);
		}
	}

	~event_fd_wrapper() noexcept
	{
		close(this->fd);
	}

	int get_fd() noexcept
	{
		return this->fd;
	}

	void set()
	{
		if (eventfd_write(this->fd, 1) < 0) {
			utki::assert(false, SL);
		}
	}

	void clear()
	{
		eventfd_t value;
		if (eventfd_read(this->fd, &value) < 0) {
			if (errno == EAGAIN) {
				return;
			}
			utki::assert(false, SL);
		}
	}
};
} // namespace
