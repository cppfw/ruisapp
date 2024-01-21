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

#include "util.hxx"

using namespace ruisapp;

version_duplet ruisapp::get_opengl_version_duplet(window_params::graphics_api api)
{
	using ga = window_params::graphics_api;
	switch (api) {
		case ga::gl_2_0:
			return {2, 0}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_2_1:
			return {2, 1}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_3_0:
			return {3, 0}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_3_1:
			return {3, 1}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_3_2:
			return {3, 2}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_3_3:
			return {3, 3}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_4_0:
			return {4, 0}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_4_1:
			return {4, 1}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_4_2:
			return {4, 2}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_4_3:
			return {4, 3}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_4_4:
			return {4, 4}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_4_5:
			return {4, 5}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		case ga::gl_4_6:
			return {4, 6}; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
		default:
			break;
	}

	std::stringstream ss;
	ss << "non-opengl api requested from opengl-based implementation of "
		  "mordavokne: window_params::graphics_api = ";
	ss << unsigned(api);

	throw std::logic_error(ss.str());
}
