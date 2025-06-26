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

#include <r4/vector.hpp>
#include <utki/flags.hpp>

namespace ruisapp {

/**
 * @brief Desired window parameters.
 */
struct window_params {
	/**
	 * @brief Desired dimensions of the window
	 */
	r4::vector2<unsigned> dims;

	/**
	 * @brief Window title.
	 */
	std::string title = "ruisapp";

	/**
	 * @brief Graphics buffer kind.
	 * Color buffer is always present, so no enum entry for color buffer is needed.
	 */
	enum class buffer {
		depth,
		stencil,

		enum_size
	};

	/**
	 * @brief Flags describing desired buffers for rendering context.
	 * Color buffer is always there implicitly.
	 */
	utki::flags<buffer> buffers = false;

	// version 0.0 means default version
	// clang-format off
	utki::version_duplet graphics_api_version = {
		.major = 0,
		.minor = 0
	};
	// clang-format on

	window_params(r4::vector2<unsigned> dims) :
		dims(dims)
	{}
};

class window
{
public:
};

} // namespace ruisapp
