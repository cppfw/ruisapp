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

#include <atomic>

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
ruis::mouse_button button_number_to_enum(uint32_t number)
{
	// from wayland's comments:
	// The button is a button code as defined in the Linux kernel's
	// linux/input-event-codes.h header file, e.g. BTN_LEFT.

	switch (number) {
		case 0x110: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::left;
		default:
		case 0x112: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::middle;
		case 0x111: // NOLINT(cppcoreguidelines-avoid-magic-numbers)
			return ruis::mouse_button::right;

			// TODO: handle

			// #define BTN_SIDE		0x113
			// #define BTN_EXTRA		0x114
			// #define BTN_FORWARD		0x115
			// #define BTN_BACK		0x116
			// #define BTN_TASK		0x117
	}
}
} // namespace

namespace {

struct window_wrapper;

window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl);
window_wrapper& get_impl(application& app);

struct window_wrapper : public utki::destructable {
	std::atomic_bool quit_flag = false;

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
		wl_seat* seat = nullptr;
		wl_pointer* pointer = nullptr;

		ruis::vector2 cur_pointer_pos{0, 0};

		constexpr static const xdg_wm_base_listener wm_base_listener = {
			.ping =
				[](void* data, xdg_wm_base* wm_base, uint32_t serial) {
					xdg_wm_base_pong(wm_base, serial);
				} //
		};

		static void wl_pointer_enter(
			void* data,
			struct wl_pointer* pointer,
			uint32_t serial,
			struct wl_surface* surface,
			wl_fixed_t x,
			wl_fixed_t y
		) //
		{
			// std::cout << "mouse enter: x,y = " << std::dec << x << ", " << y << std::endl;
			auto& self = *static_cast<registry_wrapper*>(data);
			handle_mouse_hover(ruisapp::inst(), true, 0);
			self.cur_pointer_pos = ruis::vector2(wl_fixed_to_int(x), wl_fixed_to_int(y));
			handle_mouse_move(ruisapp::inst(), self.cur_pointer_pos, 0);
		}

		static void wl_pointer_motion(void* data, struct wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
		{
			// std::cout << "mouse move: x,y = " << std::dec << x << ", " << y << std::endl;
			auto& self = *static_cast<registry_wrapper*>(data);
			self.cur_pointer_pos = ruis::vector2(wl_fixed_to_int(x), wl_fixed_to_int(y));
			handle_mouse_move(ruisapp::inst(), self.cur_pointer_pos, 0);
		}

		static void wl_pointer_button(
			void* data,
			struct wl_pointer* pointer,
			uint32_t serial,
			uint32_t time,
			uint32_t button,
			uint32_t state
		) //
		{
			// std::cout << "mouse button: " << std::hex << "0x" << button << ", state = " << "0x" << state <<
			// std::endl;
			auto& self = *static_cast<registry_wrapper*>(data);
			handle_mouse_button(
				ruisapp::inst(),
				state == WL_POINTER_BUTTON_STATE_PRESSED,
				self.cur_pointer_pos,
				button_number_to_enum(button),
				0
			);
		}

		static void wl_pointer_axis(
			void* data,
			struct wl_pointer* pointer,
			uint32_t time,
			uint32_t axis,
			wl_fixed_t value
		)
		{
			auto& self = *static_cast<registry_wrapper*>(data);

			// we get +-10 for each mouse wheel step
			auto val = wl_fixed_to_int(value);

			// std::cout << "mouse axis: " << std::dec << axis << ", val = " << val << std::endl;

			for (unsigned i = 0; i != 2; ++i) {
				handle_mouse_button(
					ruisapp::inst(),
					i == 0, // pressed/released
					self.cur_pointer_pos,
					[axis, val]() {
						if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
							if (val >= 0) {
								return ruis::mouse_button::wheel_down;
							} else {
								return ruis::mouse_button::wheel_up;
							}
						} else {
							if (val >= 0) {
								return ruis::mouse_button::wheel_right;
							} else {
								return ruis::mouse_button::wheel_left;
							}
						}
					}(),
					0 // pointer id
				);
			}
		};

		constexpr static const wl_pointer_listener pointer_listener = {
			.enter = &wl_pointer_enter,
			.leave =
				[](void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface) {
					// std::cout << "mouse leave" << std::endl;
					handle_mouse_hover(ruisapp::inst(), false, 0);
				},
			.motion = &wl_pointer_motion,
			.button = &wl_pointer_button,
			.axis = &wl_pointer_axis,
			.frame =
				[](void* data, struct wl_pointer* pointer) {
					LOG([](auto& o) {
						o << "pointer frame" << std::endl;
					})
				},
			.axis_source =
				[](void* data, struct wl_pointer* pointer, uint32_t source) {
					LOG([&](auto& o) {
						o << "axis source: " << std::dec << source << std::endl;
					})
				},
			.axis_stop =
				[](void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis) {
					LOG([&](auto& o) {
						o << "axis stop: axis = " << std::dec << axis << std::endl;
					})
				},
			.axis_discrete =
				[](void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete) {
					LOG([&](auto& o) {
						o << "axis discrete: axis = " << std::dec << axis << ", discrete = " << discrete << std::endl;
					})
				}
		};

		static void wl_seat_capabilities(void* data, struct wl_seat* wl_seat, uint32_t capabilities)
		{
			std::cout << "seat capabilities: " << std::hex << "0x" << capabilities << std::endl;

			auto& self = *static_cast<registry_wrapper*>(data);

			bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

			if (have_pointer && !self.pointer) {
				self.pointer = wl_seat_get_pointer(self.seat);
				wl_pointer_add_listener(self.pointer, &pointer_listener, &self);
			} else if (!have_pointer && self.pointer) {
				// pointer device was disconnected
				wl_pointer_release(self.pointer);
				self.pointer = nullptr;
			}
		}

		constexpr static const wl_seat_listener seat_listener =
			{.capabilities = &wl_seat_capabilities, //
			 .name = [](void* data, struct wl_seat* seat, const char* name) {
				 LOG([&](auto& o) {
					 o << "seat name: " << name << std::endl;
				 })
			 }};

		static void wl_registry_global(
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
			} else if (std::string_view(interface) == "wl_seat"sv && !self.seat) {
				void* seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
				ASSERT(seat)
				self.seat = static_cast<wl_seat*>(seat);
				wl_seat_add_listener(self.seat, &seat_listener, &self);
			}
		}

		constexpr static const wl_registry_listener listener = {
			.global = &wl_registry_global,
			.global_remove =
				[](void* data, struct wl_registry* registry, uint32_t id) {
					LOG([&](auto& o) {
						o << "got a registry losing event, id = " << id << std::endl;
					});
					// we assume that compositor and shell objects will never be removed
				} //
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
			wl_display_roundtrip(display.disp);
			wl_display_dispatch(display.disp);

			// at this point we should have compositor and shell set by global_registry_handler

			if (!this->compositor) {
				throw std::runtime_error("could not find wayland compositor");
			}

			if (!this->wm_base) {
				throw std::runtime_error("could not find xdg_shell");
			}

			if (!this->seat) {
				throw std::runtime_error("could not find wl_seat");
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
			if (this->pointer) {
				wl_pointer_release(this->pointer);
			}
			if (this->seat) {
				wl_seat_destroy(this->seat);
			}
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

		constexpr static const xdg_surface_listener listener = {
			.configure =
				[](void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
					xdg_surface_ack_configure(xdg_surface, serial);
				}, //
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
			int32_t width,
			int32_t height,
			struct wl_array* states
		)
		{
			LOG([](auto& o) {
				o << "window configure" << std::endl;
			})

			// not a window geometry event, ignore
			if (width == 0 && height == 0) {
				return;
			}

			ASSERT(width >= 0)
			ASSERT(height >= 0)

			// window resized

			LOG([](auto& o) {
				o << "window resized" << std::endl;
			})

			auto& ww = get_impl(ruisapp::inst());

			wl_egl_window_resize(ww.egl_window.win, width, height, 0, 0);
			ww.surface.commit();

			update_window_rect(ruisapp::inst(), ruis::rect(0, {ruis::real(width), ruis::real(height)}));
		}

		static void xdg_toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel)
		{
			// window closed
			auto& ww = get_impl(ruisapp::inst());
			ww.quit_flag.store(true);
		}

		constexpr static const xdg_toplevel_listener listener = {
			.configure = xdg_toplevel_handle_configure,
			.close = &xdg_toplevel_close,
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

			auto int_dims = dims.to<int>();

			// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
			this->win = wl_egl_window_create(surface.sur, int_dims.x(), int_dims.y());

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

			if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
				throw std::runtime_error("eglBindApi(OpenGL ES) failed");
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
				// unset current context
				eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

				eglDestroyContext(this->egl_display, this->egl_context);
			});

			if (eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, this->egl_context) == EGL_FALSE)
			{
				throw std::runtime_error("eglMakeCurrent() failed");
			}

			// disable v-sync
			// if (eglSwapInterval(this->egl_display, 0) != EGL_TRUE) {
			// 	throw std::runtime_error("eglSwapInterval() failed");
			// }

			scope_exit_egl_context.release();
			scope_exit_egl_window_surface.release();
			scope_exit_egl_display.release();
		}

		egl_context_wrapper(const egl_context_wrapper&) = delete;
		egl_context_wrapper& operator=(const egl_context_wrapper&) = delete;

		egl_context_wrapper(egl_context_wrapper&&) = delete;
		egl_context_wrapper& operator=(egl_context_wrapper&&) = delete;

		~egl_context_wrapper()
		{
			// unset current context
			eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

			eglDestroyContext(this->egl_display, this->egl_context);
			eglDestroySurface(this->egl_display, this->egl_surface);
			eglTerminate(this->egl_display);
		}

		void swap_frame_buffers()
		{
			eglSwapBuffers(this->egl_display, this->egl_surface);
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

window_wrapper& get_impl(application& app)
{
	return get_impl(get_window_pimpl(app));
}

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
	this->update_window_rect(ruis::rect(0, 0, ruis::real(wp.dims.x()), ruis::real(wp.dims.y())));
}

void application::swap_frame_buffers()
{
	auto& ww = get_impl(this->window_pimpl);
	ww.egl_context.swap_frame_buffers();
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
	auto& ww = get_impl(this->window_pimpl);
	ww.quit_flag.store(true);
}

int main(int argc, const char** argv)
{
	std::unique_ptr<ruisapp::application> app = create_app_unix(argc, argv);
	if (!app) {
		return 1;
	}

	auto& ww = get_impl(*app);

	while (!ww.quit_flag.load()) {
		wl_display_dispatch_pending(ww.display.disp);

		std::cout << "loop" << std::endl;

		render(*app);
	}

	return 0;
}
