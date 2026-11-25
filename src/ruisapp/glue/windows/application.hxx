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

#include <atomic>
#include <functional>
#include <utility>
#include <vector>

#include "../../application.hpp"

#ifdef RUISAPP_RENDER_VULKAN
#	include <ruis/render/vulkan/device.hpp>
#	include <ruis/render/vulkan/surface.hpp>
#	include <vulkan/vulkan.h>
#	include <vulkan/vulkan_win32.h>
#endif

#include "display.hxx"
#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
public:
	const utki::shared_ref<native_window> ruis_native_window;

	// For tracking of the mouse cursor entering or leaving the window.
	// By default we think the window is not hovered, until we receive
	// the first WM_MOUSEMOVE message.
	bool is_hovered = false;

	utki::flags<ruis::mouse_button> mouse_button_state = false;

	app_window(
		utki::shared_ref<ruis::context> ruis_context, //
		utki::shared_ref<native_window> ruis_native_window
	) :
		ruisapp::window(std::move(ruis_context)),
		ruis_native_window(std::move(ruis_native_window))
	{}

	void send_mouse_button_event(
		ruis::button_action action, //
		ruis::vec2 pos,
		ruis::mouse_button button,
		uint32_t pointer_id
	);

	/**
	 * @brief Update the ruis units (screen scale factor) from the system.
	 * Re-queries the screen scale factor reported by the system for the monitor the window
	 * is currently on and updates the units of the window's ruis context accordingly.
	 */
	void update_units_from_system();
};
} // namespace

namespace {
#ifdef RUISAPP_RENDER_VULKAN
// The Vulkan device (instance, physical device, logical device and graphics queue) is shared
// across the render factory and all the windows, the same way it is done by the other
// ruisapp glue implementations that support Vulkan (e.g. Wayland and X11).
const utki::shared_ref<ruis::render::vulkan::device> make_device()
{
	std::vector<std::string> instance_extensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
	std::vector<std::string> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	return utki::make_shared<ruis::render::vulkan::device>(
		instance_extensions, //
		device_extensions //
	);
}
#endif

class application_glue : public utki::destructable
{
	const utki::version_duplet gl_version;

	utki::shared_ref<display_wrapper> display = utki::make_shared<display_wrapper>();

	const utki::shared_ref<native_window> shared_gl_context_native_window;
#ifdef RUISAPP_RENDER_VULKAN
	// The Vulkan device is shared between the factory (resource creation) and the windows (surface creation).
	const utki::shared_ref<ruis::render::vulkan::device> device;
#endif
	// concrete factory type, #ifdef by renderer
#ifdef RUISAPP_RENDER_VULKAN
	const utki::shared_ref<const ruis::render::vulkan::factory> render_factory;
#elif defined(RUISAPP_RENDER_OPENGL)
	const utki::shared_ref<const ruis::render::opengl::factory> render_factory;
#elif defined(RUISAPP_RENDER_OPENGLES)
	const utki::shared_ref<const ruis::render::opengles::factory> render_factory;
#else
#	error "Unknown graphics API"
#endif
	const utki::shared_ref<const ruis::render::factory::shaders> common_shaders;
	const utki::shared_ref<const ruis::render::renderer::objects> common_render_objects;
	const utki::shared_ref<ruis::resource_loader> ruis_resource_loader;

public:
	const utki::shared_ref<ruis::style_provider> ruis_style_provider;

private:
	// std::vector is used instead of std::map to preserve the order in which
	// the windows were created, so that application::get_windows() returns the
	// windows in a stable order. The number of windows is small (2-3), so
	// linear lookup by window id is fast enough.
	std::vector<std::pair<native_window::window_id_type, utki::shared_ref<app_window>>> windows;

public:
	std::atomic_bool quit_flag = false;

	const utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

	std::vector<utki::shared_ref<app_window>> windows_to_destroy;

	const auto& get_windows() const noexcept
	{
		return this->windows;
	}

	application_glue(const utki::version_duplet& gl_version);

	ruisapp::window& make_window(ruisapp::window_parameters window_params);
	void destroy_window(native_window::window_id_type id);

	auto find_window(native_window::window_id_type id);

	app_window* get_window(native_window::window_id_type id);

	void render();

	// posts a procedure to be executed on the UI thread's message queue
	void post_to_ui_thread(std::function<void()> procedure);
};
} // namespace

namespace {
inline application_glue& get_glue(ruisapp::application& app)
{
	return static_cast<application_glue&>(app.pimpl.get());
}
} // namespace

namespace {
inline application_glue& get_glue()
{
	return get_glue(ruisapp::application::inst());
}
} // namespace
