/*
mordavokne - morda GUI adaptation layer

Copyright (C) 2016-2021  Ivan Gagis <igagis@gmail.com>

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

#include "../application.hpp"

namespace {

std::unique_ptr<ruisapp::application> create_app_unix(int argc, const char** argv)
{
	return ruisapp::application_factory::get_factory()(utki::make_span(argv, argc));
}

std::string initialize_storage_dir(const std::string& app_name)
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
	auto home_dir = getenv("HOME");
	if (!home_dir) {
		throw std::runtime_error("failed to get user home directory. Is HOME environment variable set?");
	}

	std::string home_dir_str(home_dir);

	if (*home_dir_str.rend() != '/') {
		home_dir_str.append(1, '/');
	}

	home_dir_str.append(1, '.').append(app_name).append(1, '/');

	papki::fs_file dir(home_dir_str);
	if (!dir.exists()) {
		dir.make_dir();
	}

	return home_dir_str;
}

} // namespace
