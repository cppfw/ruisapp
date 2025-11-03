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

#include "../application.hpp"

#ifdef assert
#	undef assert
#endif

namespace {

inline std::string get_xdg_dir_home(
	const char* xdg_env_var, //
	std::string_view default_subdir,
	std::string_view app_name
)
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
	if (auto xdg_dir = getenv(xdg_env_var)) {
		// the directory is explicitly set via XDG_*_HOME environment variable, return the value
		return utki::cat(
			fsif::as_dir(xdg_dir), //
			app_name,
			'/'
		);
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
	auto home_dir = getenv("HOME");
	if (!home_dir) {
		throw std::runtime_error("failed to get user home directory. Is HOME environment variable set?");
	}

	utki::assert(fsif::is_dir(default_subdir), SL);
	return utki::cat(
		fsif::as_dir(home_dir), //
		default_subdir,
		app_name,
		'/'
	);
}

inline ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	ruisapp::application::directories dirs;

	using namespace std::string_view_literals;

	dirs.cache = get_xdg_dir_home("XDG_CACHE_HOME", ".cache/"sv, app_name);
	dirs.config = get_xdg_dir_home("XDG_CONFIG_HOME", ".config/"sv, app_name);
	dirs.state = get_xdg_dir_home("XDG_STATE_HOME", ".local/state/"sv, app_name);

	// std::cout << "cache dir = " << dirs.cache << std::endl;
	// std::cout << "config dir = " << dirs.config << std::endl;
	// std::cout << "state dir = " << dirs.state << std::endl;

	return dirs;
}

} // namespace
