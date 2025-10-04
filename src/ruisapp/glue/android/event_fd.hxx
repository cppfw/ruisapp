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
