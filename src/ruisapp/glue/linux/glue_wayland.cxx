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
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

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

		display_wrapper() :
			disp(wl_display_connect(nullptr))
		{
			if (!this->disp) {
				throw std::runtime_error("could not connect to wayland display");
			}
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

	struct registry_wrapper {
		wl_registry* reg;

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
			auto& self = *static_cast<registry_wrapper*>(data);

			LOG([&](auto& o) {
				o << "got a registry event for: " << interface << ", id = " << id << std::endl;
			});
			if (std::string_view(interface) == "wl_compositor"sv && !self.compositor) {
				void* compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
				ASSERT(compositor)
				self.compositor = static_cast<wl_compositor*>(compositor);
			} else if (std::string_view(interface) == xdg_wm_base_interface.name && !self.wm_base) {
				void* wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
				ASSERT(wm_base)
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
			// we assume that compositor and shell objects will never be removed
		}

		constexpr static const wl_registry_listener listener = {
			.global = &global_registry_handler, //
			.global_remove = &global_registry_remover
		};

		registry_wrapper(display_wrapper& display) :
			reg(wl_display_get_registry(display.disp))
		{
			if (!this->reg) {
				throw std::runtime_error("could not create wayland registry");
			}
			utki::scope_exit registry_scope_exit([this]() {
				this->destroy();
			});

			wl_registry_add_listener(this->reg, &listener, this);

			// this will call the attached listener's global_registry_handler
			wl_display_dispatch(display.disp);
			wl_display_roundtrip(display.disp);

			// at this point we should have compositor and shell set by global_registry_handler

			if (!this->compositor) {
				throw std::runtime_error("could not find wayland compositor");
			}

			if (!this->wm_base) {
				throw std::runtime_error("could not find xdg_shell");
			}

			registry_scope_exit.release();
		}

		registry_wrapper(const registry_wrapper&) = delete;
		registry_wrapper& operator=(const registry_wrapper&) = delete;

		registry_wrapper(registry_wrapper&&) = delete;
		registry_wrapper& operator=(registry_wrapper&&) = delete;

		~registry_wrapper()
		{
			this->destroy();
		}

	private:
		void destroy()
		{
			if (this->wm_base) {
				xdg_wm_base_destroy(this->wm_base);
			}
			if (this->compositor) {
				wl_compositor_destroy(this->compositor);
			}
			wl_registry_destroy(this->reg);
		}
	} registry;

	struct region_wrapper {
		wl_region* reg;

		region_wrapper(registry_wrapper& registry) :
			reg(wl_compositor_create_region(registry.compositor))
		{
			if (!this->reg) {
				throw std::runtime_error("could not create wayland region");
			}
		}

		region_wrapper(const region_wrapper&) = delete;
		region_wrapper& operator=(const region_wrapper&) = delete;

		region_wrapper(region_wrapper&&) = delete;
		region_wrapper& operator=(region_wrapper&&) = delete;

		~region_wrapper()
		{
			wl_region_destroy(this->reg);
		}

		void add(r4::rectangle<int32_t> rect)
		{
			wl_region_add(this->reg, rect.p.x(), rect.p.y(), rect.d.x(), rect.d.y());
		}
	};

	struct surface_wrapper {
		wl_surface* sur;

		surface_wrapper(registry_wrapper& registry) :
			sur(wl_compositor_create_surface(registry.compositor))
		{
			if (!this->sur) {
				throw std::runtime_error("could not create wayland surface");
			}
		}

		surface_wrapper(const surface_wrapper&) = delete;
		surface_wrapper& operator=(const surface_wrapper&) = delete;

		surface_wrapper(surface_wrapper&&) = delete;
		surface_wrapper& operator=(surface_wrapper&&) = delete;

		~surface_wrapper()
		{
			wl_surface_destroy(this->sur);
		}

		void commit()
		{
			wl_surface_commit(this->sur);
		}

		void set_opaque_region(const region_wrapper& region)
		{
			wl_surface_set_opaque_region(this->sur, region.reg);
		}
	} surface;

	struct xdg_surface_wrapper {
		xdg_surface* xdg_sur;

		static void xdg_surface_configure( //
			void* data,
			struct xdg_surface* xdg_surface,
			uint32_t serial
		)
		{
			xdg_surface_ack_configure(xdg_surface, serial);
		}

		constexpr static const xdg_surface_listener listener = {
			.configure = xdg_surface_configure,
		};

		xdg_surface_wrapper(surface_wrapper& surface, registry_wrapper& registry) :
			xdg_sur(xdg_wm_base_get_xdg_surface(registry.wm_base, surface.sur))
		{
			if (!xdg_sur) {
				throw std::runtime_error("could not create wayland xdg surface");
			}

			xdg_surface_add_listener(this->xdg_sur, &listener, nullptr);
		}

		xdg_surface_wrapper(const xdg_surface_wrapper&) = delete;
		xdg_surface_wrapper& operator=(const xdg_surface_wrapper&) = delete;

		xdg_surface_wrapper(xdg_surface_wrapper&&) = delete;
		xdg_surface_wrapper& operator=(xdg_surface_wrapper&&) = delete;

		~xdg_surface_wrapper()
		{
			xdg_surface_destroy(this->xdg_sur);
		}
	} xdg_surface;

	struct toplevel_wrapper {
		xdg_toplevel* toplev;

		static void xdg_toplevel_handle_configure(
			void* data,
			struct xdg_toplevel* xdg_toplevel,
			int32_t w,
			int32_t h,
			struct wl_array* states
		)
		{
			// no window geometry event, ignore
			if (w == 0 && h == 0)
				return;

			// window resized

			// TODO:
			// if(old_w != w && old_h != h) {
			// 	old_w = w;
			// 	old_h = h;

			// wl_egl_window_resize(ESContext.native_window, w, h, 0, 0);
			// wl_surface_commit(surface);
			// }
		}

		static void xdg_toplevel_handle_close(void* data, struct xdg_toplevel* xdg_toplevel)
		{
			// window closed, be sure that this event gets processed
			// program_alive = false;
		}

		constexpr static const xdg_toplevel_listener listener = {
			.configure = xdg_toplevel_handle_configure,
			.close = xdg_toplevel_handle_close,
		};

		toplevel_wrapper(surface_wrapper& surface, xdg_surface_wrapper& xdg_surface) :
			toplev(xdg_surface_get_toplevel(xdg_surface.xdg_sur))
		{
			if (!this->toplev) {
				throw std::runtime_error("could not get wayland xdg toplevel");
			}

			xdg_toplevel_set_title(this->toplev, "Wayland EGL example");
			xdg_toplevel_add_listener(this->toplev, &listener, nullptr);

			surface.commit();
		}

		toplevel_wrapper(const toplevel_wrapper&) = delete;
		toplevel_wrapper& operator=(const toplevel_wrapper&) = delete;

		toplevel_wrapper(toplevel_wrapper&&) = delete;
		toplevel_wrapper& operator=(toplevel_wrapper&&) = delete;

		~toplevel_wrapper()
		{
			xdg_toplevel_destroy(this->toplev);
		}
	} toplevel;

	region_wrapper region;

	struct egl_window_wrapper {
		wl_egl_window* win;

		egl_window_wrapper(surface_wrapper& surface, region_wrapper& region, r4::vector2<unsigned> dims)
		{
			region.add(r4::rectangle({0, 0}, dims.to<int32_t>()));
			surface.set_opaque_region(region);

			this->win = wl_egl_window_create(surface.sur, dims.x(), dims.y());

			if (!this->win) {
				throw std::runtime_error("could not create wayland egl window");
			}
		}

		egl_window_wrapper(const egl_window_wrapper&) = delete;
		egl_window_wrapper& operator=(const egl_window_wrapper&) = delete;

		egl_window_wrapper(egl_window_wrapper&&) = delete;
		egl_window_wrapper& operator=(egl_window_wrapper&&) = delete;

		~egl_window_wrapper()
		{
			wl_egl_window_destroy(this->win);
		}
	} egl_window;

	struct egl_context_wrapper {
		EGLDisplay egl_display;
		EGLSurface egl_surface;
		EGLContext egl_context;

		egl_context_wrapper(
			const display_wrapper& display,
			const egl_window_wrapper& egl_window,
			const window_params& wp
		) :
			egl_display(eglGetDisplay(display.disp))
		{
			if (this->egl_display == EGL_NO_DISPLAY) {
				throw std::runtime_error("could not open EGL display");
			}

			utki::scope_exit scope_exit_egl_display([this]() {
				eglTerminate(this->egl_display);
			});

			if (!eglInitialize(this->egl_display, nullptr, nullptr)) {
				throw std::runtime_error("could not initialize EGL");
			}

			EGLConfig egl_config = nullptr;
			{
				// Here specify the attributes of the desired configuration.
				// Below, we select an EGLConfig with at least 8 bits per color
				// component compatible with on-screen windows.
				const std::array<EGLint, 15> attribs = {
					EGL_SURFACE_TYPE,
					EGL_WINDOW_BIT,
					EGL_RENDERABLE_TYPE,
					EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT,
					EGL_BLUE_SIZE,
					8,
					EGL_GREEN_SIZE,
					8,
					EGL_RED_SIZE,
					8,
					EGL_DEPTH_SIZE,
					wp.buffers.get(window_params::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
					EGL_STENCIL_SIZE,
					wp.buffers.get(window_params::buffer::stencil) ? utki::byte_bits : 0,
					EGL_NONE
				};

				// Here, the application chooses the configuration it desires. In this
				// sample, we have a very simplified selection process, where we pick
				// the first EGLConfig that matches our criteria.
				EGLint num_configs = 0;
				eglChooseConfig(this->egl_display, attribs.data(), &egl_config, 1, &num_configs);
				if (num_configs <= 0) {
					throw std::runtime_error("eglChooseConfig() failed, no matching config found");
				}
			}

			// TODO: is this needed?
			if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
				throw std::runtime_error("eglBindApi() failed");
			}

			this->egl_surface = eglCreateWindowSurface(this->egl_display, egl_config, egl_window.win, nullptr);
			if (this->egl_surface == EGL_NO_SURFACE) {
				throw std::runtime_error("could not create EGL window surface");
			}

			utki::scope_exit scope_exit_egl_window_surface([this]() {
				eglDestroySurface(this->egl_display, this->egl_surface);
			});

			auto graphics_api_version = [&ver = wp.graphics_api_version]() {
				if (ver.to_uint32_t() == 0) {
					// default OpenGL ES version is 2.0
					return utki::version_duplet{
						.major = 2, //
						.minor = 0
					};
				}
				return ver;
			}();

			{
				constexpr auto attrs_array_size = 5;
				std::array<EGLint, attrs_array_size> context_attrs = {
					EGL_CONTEXT_MAJOR_VERSION,
					graphics_api_version.major,
					EGL_CONTEXT_MINOR_VERSION,
					graphics_api_version.minor,
					EGL_NONE
				};

				this->egl_context =
					eglCreateContext(this->egl_display, egl_config, EGL_NO_CONTEXT, context_attrs.data());
				if (this->egl_context == EGL_NO_CONTEXT) {
					throw std::runtime_error("could not create EGL context");
				}
			}

			utki::scope_exit scope_exit_egl_context([this]() {
				eglDestroyContext(this->egl_display, this->egl_context);
			});

			if (eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context) == EGL_FALSE)
			{
				throw std::runtime_error("eglMakeCurrent() failed");
			}

			scope_exit_egl_context.release();
			scope_exit_egl_window_surface.release();
			scope_exit_egl_display.release();
		}

		~egl_context_wrapper()
		{
			// unset current context
			eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

			eglDestroyContext(this->egl_display, this->egl_context);
			eglDestroySurface(this->egl_display, this->egl_surface);
			eglTerminate(this->egl_display);
		}
	} egl_context;

	window_wrapper(const window_params& wp) :
		registry(this->display),
		surface(this->registry),
		xdg_surface(this->surface, this->registry),
		toplevel(this->surface, this->xdg_surface),
		region(this->registry),
		egl_window(this->surface, this->region, wp.dims),
		egl_context(this->display, this->egl_window, wp)
	{}

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
