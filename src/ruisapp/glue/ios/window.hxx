#pragma once

#import <GLKit/GLKit.h>
#import <UIKit/UIKit.h>

#ifdef assert
#	undef assert
#endif

#include <ruis/render/native_window.hpp>

namespace {
class app_window;
} // namespace

@interface RuisappViewController : GLKViewController <GLKViewDelegate> {
@public
	EAGLContext* context;
	decltype(ruisapp::window_parameters::buffers) buffers;
	app_window* window;
}

- (id)init;

@end

namespace {
class native_window : public ruis::render::native_window
{
	struct ios_egl_context_wrapper {
		EAGLContext* const context;

		ios_egl_context_wrapper(const utki::version_duplet& gl_version);

		ios_egl_context_wrapper(const ios_egl_context_wrapper&) = delete;
		ios_egl_context_wrapper& operator=(const ios_egl_context_wrapper&) = delete;

		ios_egl_context_wrapper(ios_egl_context_wrapper&&) = delete;
		ios_egl_context_wrapper& operator=(ios_egl_context_wrapper&&) = delete;

		~ios_egl_context_wrapper();
	} ios_egl_context;

	struct ios_view_controller_wrapper {
		RuisappViewController* const view_controller;

		ios_view_controller_wrapper(
			ios_egl_context_wrapper& ios_egl_context, //
			const decltype(ruisapp::window_parameters::buffers)& buffers
		);

		ios_view_controller_wrapper(const ios_view_controller_wrapper&) = delete;
		ios_view_controller_wrapper& operator=(const ios_view_controller_wrapper&) = delete;

		ios_view_controller_wrapper(ios_view_controller_wrapper&&) = delete;
		ios_view_controller_wrapper& operator=(ios_view_controller_wrapper&&) = delete;

		~ios_view_controller_wrapper();
	} ios_view_controller;

	struct ios_window_wrapper {
		UIWindow* const window;

		ios_window_wrapper(ios_view_controller_wrapper& ios_view_controller);

		ios_window_wrapper(const ios_window_wrapper&) = delete;
		ios_window_wrapper& operator=(const ios_window_wrapper&) = delete;

		ios_window_wrapper(ios_window_wrapper&&) = delete;
		ios_window_wrapper& operator=(ios_window_wrapper&&) = delete;

		~ios_window_wrapper();
	} ios_window;

public:
	native_window(
		const utki::version_duplet& gl_version, //
		ruisapp::window_parameters window_params
	);

	void set_app_window(app_window* w);

	void swap_frame_buffers() override;

	void bind_rendering_context() override;

	ruis::rect get_content_rect() const;
};
} // namespace
