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

#include <ruis/render/native_window.hpp>

#include "../../window.hpp"
#include "../egl_utils.hxx"

namespace {
class native_window : public ruis::render::native_window
{
	egl_display_wrapper egl_display;
	egl_config_wrapper egl_config;
	egl_context_wrapper egl_context;

	std::optional<egl_surface_wrapper> egl_surface;

	std::optional<egl_pbuffer_surface_wrapper> egl_dummy_surface;

public:
	native_window(
		const utki::version_duplet& gl_version, //
		const ruisapp::window_parameters& window_params
	);

	void swap_frame_buffers() override;

	void bind_rendering_context() override;

	void create_surface(ANativeWindow& android_window);
	void destroy_surface();

	r4::vector2<unsigned> get_dims();

	void set_fullscreen_internal(bool enable) override;

	void set_virtual_keyboard_visible(bool visible) noexcept override;
};
} // namespace
