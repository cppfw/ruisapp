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

#include <functional>

#include <r4/vector.hpp>
#include <ruis/gui.hpp>
#include <utki/flags.hpp>

namespace ruisapp {

/**
 * @brief Widnow orientation policy.
 */
enum class orientation {
	/**
	 * @brief Switch orientation when screen orientation changes.
	 */
	dynamic,

	/**
	 * @brief Stay always in landscape orientation.
	 */
	landscape,

	/**
	 * @brief Stay always in portrait orientation.
	 */
	portrait,

	enum_size
};

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
 * @brief Desired window parameters.
 */
struct window_parameters {
	constexpr const static auto default_window_width = 300;
	constexpr const static auto default_window_height = 150;

	/**
	 * @brief Desired dimensions of the window
	 */
	r4::vector2<unsigned> dims = {default_window_width, default_window_height};

	/**
	 * @brief Window title.
	 */
	std::string title = "ruisapp";

	/**
	 * @brief Monitor index to place the window on.
	 * Index of the monitor to initially place the window on.
	 * The indexing starts from 1.
	 * Value of 0 means to pick monitor automatically.
	 * If the index exceeds the number of monitors the system actually has then
	 * the window is placed to a monitor picked automatically as if the index was 0.
	 */
	unsigned monitor = 0;

	/**
	 * @brief Orientation policy.
	 */
	ruisapp::orientation orientation = ruisapp::orientation::dynamic;

	/**
	 * @brief Indicates that the window should be created initially fullscreen.
	 */
	// TODO: implement support for all backends
	bool fullscreen = false;

	/**
	 * @brief Indicates whether the window is initially visible or not.
	 */
	bool visible = true;

	/**
	 * @brief Indicates that the window is added to the taskbar.
	 */
	bool taskbar = true;

	/**
	 * @brief Flags describing desired buffers for rendering context.
	 * Color buffer is always there implicitly.
	 */
	utki::flags<ruisapp::buffer> buffers = false;
};

class window
{
	// TODO: only needed on windows, move to window implementation?
	r4::rectangle<int> before_fullscreen_window_rect{0, 0, 0, 0};

public:
	ruis::gui gui;

	window(utki::shared_ref<ruis::context> ruis_context);

	window(const window&) = delete;
	window& operator=(const window&) = delete;

	window(window&&) = delete;
	window& operator=(window&&) = delete;

	virtual ~window() = default;

	void render();
};

} // namespace ruisapp
