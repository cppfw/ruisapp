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

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

#include "wayland_window.hxx"

namespace {
// Vulkan-specific native window for Wayland.
class vulkan_native_window : public wayland_window
{
public:
	vulkan_native_window(
		utki::shared_ref<display_wrapper> display,
		const ruisapp::window_parameters& window_params
	) :
		wayland_window(
			std::move(display), //
			window_params
		)
	{}

	void bind_rendering_context() override
	{
		// For Vulkan, there's no concept of binding a rendering context.
		// The Vulkan context is thread-safe and doesn't need to be made current.
	}

	bool is_rendering_context_bound() const noexcept override
	{
		// For Vulkan, the context is always "bound" (thread-safe)
		return true;
	}

	void set_vsync_enabled_internal(bool enabled) override
	{
		(void)enabled;
		// VSync is handled by the Vulkan present mode,
		// see pick_present_mode() in ruis-render-vulkan/src/ruis/render/vulkan/context.cpp
	}
};
} // namespace
