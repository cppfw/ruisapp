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
};
} // namespace
