/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2024  Ivan Gagis <igagis@gmail.com>

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

#include <papki/fs_file.hpp>
#include <utki/destructable.hpp>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#ifdef RUISAPP_RENDER_OPENGL
#	include <GL/glew.h>
// #	include <GL/glx.h>
#	include <ruis/render/opengl/renderer.hpp>

#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <GLES2/gl2.h>
#	include <ruis/render/opengles/renderer.hpp>

#else
#	error "Unknown graphics API"
#endif

#include <xdg-shell-client-protocol.h>

#include "../../application.hpp"
#include "../friend_accessors.cxx" // NOLINT(bugprone-suspicious-include)
#include "../unix_common.cxx" // NOLINT(bugprone-suspicious-include)

using namespace std::string_view_literals;

using namespace ruisapp;

namespace {
struct window_wrapper : public utki::destructable {
	struct display_wrapper {
		wl_display* disp;

		wl_compositor* compositor = nullptr;
		xdg_wm_base* wm_base = nullptr;

		static void xdg_wm_base_ping(void* data, xdg_wm_base* wm_base, uint32_t serial)
		{
			xdg_wm_base_pong(wm_base, serial);
		}

		constexpr static const xdg_wm_base_listener wm_base_listener = {
			.ping = &xdg_wm_base_ping //
		};

		static void global_registry_handler( //
			void* data,
			wl_registry* registry,
			uint32_t id,
			const char* interface,
			uint32_t version
		)
		{
			ASSERT(data)
			auto& self = *static_cast<display_wrapper*>(data);

			LOG([&](auto& o) {
				o << "got a registry event for: " << interface << ", id = " << id << std::endl;
			});
			if (std::string_view(interface) == "wl_compositor"sv) {
				void* compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
				self.compositor = static_cast<wl_compositor*>(compositor);
			} else if (std::string_view(interface) == xdg_wm_base_interface.name) {
				void* wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
				self.wm_base = static_cast<xdg_wm_base*>(wm_base);
				xdg_wm_base_add_listener(self.wm_base, &wm_base_listener, nullptr);
			}
		}

		static void global_registry_remover( //
			void* data,
			struct wl_registry* registry,
			uint32_t id
		)
		{
			LOG([&](auto& o) {
				o << "got a registry losing event, id = " << id << std::endl;
			});
		}

		constexpr static const wl_registry_listener listener = {
			.global = &global_registry_handler, //
			.global_remove = &global_registry_remover
		};

		display_wrapper() :
			disp(wl_display_connect(nullptr))
		{
			if (!this->disp) {
				throw std::runtime_error("could not connect to wayland display");
			}

			wl_registry* registry = wl_display_get_registry(this->disp);
			ASSERT(registry)
			wl_registry_add_listener(registry, &listener, this);

			// this will call the attached listener global_registry_handler
			wl_display_dispatch(this->disp);
			wl_display_roundtrip(this->disp);
		}

		display_wrapper(const display_wrapper&) = delete;
		display_wrapper& operator=(const display_wrapper&) = delete;

		display_wrapper(display_wrapper&&) = delete;
		display_wrapper& operator=(display_wrapper&&) = delete;

		~display_wrapper()
		{
			wl_display_disconnect(this->disp);
		}
	} display;

	window_wrapper(const window_params& wp) {}

	ruis::real get_dots_per_inch()
	{
		// TODO:
		return 96; // NOLINT

		// int src_num = 0;

		// constexpr auto mm_per_cm = 10;

		// ruis::real value =
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	((ruis::real(DisplayWidth(this->display.display, src_num))
		// 	  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	  / (ruis::real(DisplayWidthMM(this->display.display, src_num)) / ruis::real(mm_per_cm)))
		// 	 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	 +
		// 	 // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	 (ruis::real(DisplayHeight(this->display.display, src_num))
		// 	  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	  / (ruis::real(DisplayHeightMM(this->display.display, src_num)) / ruis::real(mm_per_cm)))) /
		// 	2;
		// constexpr auto cm_per_inch = 2.54;
		// value *= ruis::real(cm_per_inch);
		// return value;
	}

	ruis::real get_dots_per_pp()
	{
		// TODO:
		return 1;

		// // TODO: use scale factor only for desktop monitors
		// if (this->scale_factor != ruis::real(1)) {
		// 	return this->scale_factor;
		// }

		// int src_num = 0;
		// r4::vector2<unsigned> resolution(
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayWidth(this->display.display, src_num),
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayHeight(this->display.display, src_num)
		// );
		// r4::vector2<unsigned> screen_size_mm(
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayWidthMM(this->display.display, src_num),
		// 	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
		// 	DisplayHeightMM(this->display.display, src_num)
		// );

		// return application::get_pixels_per_pp(resolution, screen_size_mm);
	}
};

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
{
	ASSERT(dynamic_cast<window_wrapper*>(pimpl.get()))
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
	return static_cast<window_wrapper&>(*pimpl);
}

// TODO:
// window_wrapper& get_impl(application& app)
// {
// 	return get_impl(get_window_pimpl(app));
// }

} // namespace

application::application(std::string name, const window_params& wp) :
	name(std::move(name)),
	window_pimpl(std::make_unique<window_wrapper>(wp)),
	gui(utki::make_shared<ruis::context>(
#ifdef RUISAPP_RENDER_OPENGL
		utki::make_shared<ruis::render_opengl::renderer>(),
#elif defined(RUISAPP_RENDER_OPENGLES)
		utki::make_shared<ruis::render_opengles::renderer>(),
#else
#	error "Unknown graphics API"
#endif
		utki::make_shared<ruis::updater>(),
		[](std::function<void()> proc) {
			// TODO:
			// get_impl(get_window_pimpl(*this)).ui_queue.push_back(std::move(a));
		},
		[](ruis::mouse_cursor c) {
			// TODO:
			// auto& ww = get_impl(*this);
			// ww.set_cursor(c);
		},
		get_impl(window_pimpl).get_dots_per_inch(),
		get_impl(window_pimpl).get_dots_per_pp()
	)),
	storage_dir(initialize_storage_dir(this->name))
{
	// TODO:
}

void application::swap_frame_buffers()
{
	// TODO:
}

void application::set_mouse_cursor_visible(bool visible)
{
	// TODO:
}

void application::set_fullscreen(bool fullscreen)
{
	// TODO:
}

void ruisapp::application::quit() noexcept
{
	// TODO:
}

int main(int argc, const char** argv)
{
	std::unique_ptr<ruisapp::application> app = create_app_unix(argc, argv);
	if (!app) {
		return 1;
	}

	// TODO:

	return 0;
}
