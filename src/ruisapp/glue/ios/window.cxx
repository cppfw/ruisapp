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

#include "window.hxx"

#include "application.hxx"

namespace {
void handle_mouse_button(
	button_action action, //
	const ruis::vec2& pos,
	ruis::mouse_button button,
	unsigned pointer_id
)
{
	auto& glue = get_glue();

	auto* win = glue.get_window();
	if (!win) {
		return;
	}

	win->gui.send_mouse_button(
		action, //
		pos,
		button,
		pointer_id
	);
}
} // namespace

namespace {
void handle_mouse_move(
	const ruis::vec2& pos, //
	unsigned pointer_id
)
{
	auto& glue = get_glue();

	auto* win = glue.get_window();
	if (!win) {
		return;
	}

	win->gui.send_mouse_move(pos, pointer_id);
}
} // namespace

@implementation RuisappViewController

- (id)init
{
	self = [super init];

	if (!self) {
		return nil;
	}

	self->context = nullptr;
	self->window = nullptr;
	self->buffers.clear();

	return self;
}

- (void)viewDidLoad
{
	[super viewDidLoad];

	GLKView* view = (GLKView*)self.view;

	// set EGL context to the GLKView
	utki::assert(self->context, SL);
	view.context = self->context;

	{
		view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;

		auto& buffers = self->buffers;

		if (buffers.get(ruisapp::buffer::depth)) {
			view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
		} else {
			view.drawableDepthFormat = GLKViewDrawableDepthFormatNone;
		}
		if (buffers.get(ruisapp::buffer::stencil)) {
			view.drawableStencilFormat = GLKViewDrawableStencilFormat8;
		} else {
			view.drawableStencilFormat = GLKViewDrawableStencilFormatNone;
		}
	}

	[EAGLContext setCurrentContext:self->context];

	view.multipleTouchEnabled = YES;

	// TODO: what is it?
	if ([self respondsToSelector:@selector(edgesForExtendedLayout)]) {
		self.edgesForExtendedLayout = UIRectEdgeNone;
	}
}

- (void)dealloc
{
	[super dealloc];

	if ([EAGLContext currentContext] == self->context) {
		[EAGLContext setCurrentContext:nil];
	}
}

- (void)didReceiveMemoryWarning
{
	// Dispose of any resources that can be recreated.
}

- (void)update
{
	// TODO: updater is not tied to a window, but on ios we update it from the ViewController which looks incorrect.
	//       Investigate if it is possible to move it out of window code somehow. Perhaps will have to use soemthing else than GLKViewController.
	auto& glue = get_glue();
	// TODO: adapt to nothing to update, lower frame rate
	glue.updater.get().update();
}

- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect
{
	if (!self.view) {
		// the viewDidLoad has not yet been called and the view is not initialized, cannot render
		return;
	}

	utki::assert(self->window, SL);

	auto& natwin = self->window->ruis_native_window.get();

	// TODO: get correct content rect
	auto content_rect = natwin.get_content_rect();
	// ruis::rect content_rect{
	//     {ruis::real(rect.origin.x), ruis::real(rect.origin.y)},
	//     {ruis::real(rect.size.width), ruis::real(rect.size.height)} //
	// };

	// utki::log_debug([&](auto&o){
	//     o << "content_rect = " << content_rect << std::endl;
	// });

	// TODO: for optimization, check if rect has changed
	// set the GL viewport
	self->window->gui.set_viewport(content_rect);

	auto& glue = get_glue();
	glue.render();
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	float scale = [UIScreen mainScreen].scale;

	for (UITouch* touch in touches) {
		CGPoint p = [touch locationInView:self.view];

		using std::round;
		handle_mouse_button(
			ruis::button_action::press, //
			round(ruis::vec2(p.x * scale, p.y * scale)), // pos
			ruis::mouse_button::left,
			0 // TODO: pointer id
		);
	}
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	float scale = [UIScreen mainScreen].scale;

	for (UITouch* touch in touches) {
		CGPoint p = [touch locationInView:self.view];

		using std::round;
		handle_mouse_move(
			round(ruis::vec2(p.x * scale, p.y * scale)), // pos
			0 // TODO: pointer id
		);
	}
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	float scale = [UIScreen mainScreen].scale;

	for (UITouch* touch in touches) {
		CGPoint p = [touch locationInView:self.view];

		using std::round;
		handle_mouse_button(
			ruis::button_action::release, //
			round(ruis::vec2(p.x * scale, p.y * scale)), // pos
			ruis::mouse_button::left,
			0 // TODO: pointer id
		);
	}
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	// TODO:
}

@end // ViewController implementation

native_window::ios_egl_context_wrapper::ios_egl_context_wrapper(const utki::version_duplet& gl_version) :
	context([&]() {
		EAGLContext* ctx = nullptr;
		if (gl_version.major >= 4) {
			throw std::logic_error(utki::cat(
				"ios_egl_context_wrapper::ios_egl_context_wrapper(): unknown OpenGL ES version requested: ",
				gl_version
			));
		} else if (gl_version.major == 3) {
			ctx = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
		} else {
			ctx = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		}

		if (!ctx) {
			throw std::runtime_error(
				"ios_egl_context_wrapper::ios_egl_context_wrapper(): could not create OpenGL ES context"
			);
		}

		return ctx;
	}())
{}

native_window::ios_egl_context_wrapper::~ios_egl_context_wrapper()
{
	[this->context release];
}

native_window::ios_view_controller_wrapper::ios_view_controller_wrapper(
	ios_egl_context_wrapper& ios_egl_context, //
	const decltype(ruisapp::window_parameters::buffers)& buffers
) :
	view_controller([&]() {
		auto* vc = [[RuisappViewController alloc] init];

		if (!vc) {
			throw std::runtime_error(
				"ios_view_controller_wrapper::ios_view_controller_wrapper(): could not create view controller"
			);
		}

		vc->context = ios_egl_context.context;
		vc->buffers = buffers;

		return vc;
	}())
{}

native_window::ios_view_controller_wrapper::~ios_view_controller_wrapper()
{
	[this->view_controller release];
}

native_window::ios_window_wrapper::ios_window_wrapper(ios_view_controller_wrapper& ios_view_controller) :
	window([&]() {
		auto* win = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

		if (!win) {
			throw std::runtime_error("failed to create a UIWindow");
		}

		utki::scope_exit scope_exit_window([&]() {
			[win release];
		});

		// here in Objective-C the . operator invokes getter/setter
		win.screen = [UIScreen mainScreen];
		win.backgroundColor = [UIColor redColor];
		win.rootViewController = ios_view_controller.view_controller;

		[win makeKeyAndVisible];

		scope_exit_window.release();

		return win;
	}())
{}

native_window::ios_window_wrapper::~ios_window_wrapper()
{
	[this->window release];
}

native_window::native_window(
	const utki::version_duplet& gl_version, //
	ruisapp::window_parameters window_params
) :
	ios_egl_context(gl_version),
	ios_view_controller(
		this->ios_egl_context, //
		window_params.buffers
	),
	ios_window(this->ios_view_controller)
{}

void native_window::set_app_window(app_window* w)
{
	utki::assert(!this->ios_view_controller.view_controller->window, SL);
	this->ios_view_controller.view_controller->window = w;
}

void native_window::swap_frame_buffers()
{
	// do nothing, GLKViewController will swap buffers for us
}

void native_window::bind_rendering_context()
{
	[EAGLContext setCurrentContext:this->ios_egl_context.context];
}

void native_window::set_fullscreen_internal(bool enable)
{
	float scale = [[UIScreen mainScreen] scale];

	using std::round;

	if (enable) {
		if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0f) {
			CGRect rect = this->ios_window.window.frame;
			this->ios_window.window.rootViewController.view.frame = rect;
		}

		// TODO: this was setting the viewport, is something still needed here?
		// update_window_rect(
		// 		ruis::rect(
		// 				ruis::vec2(0),
		// 				ruis::vec2(
		// 						round(this->ios_window.window.frame.size.width * scale),
		// 						round(this->ios_window.window.frame.size.height * scale)
		// 					)
		// 			)
		// 	);

		this->ios_window.window.windowLevel = UIWindowLevelStatusBar;
	} else {
		CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;

		if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0f) {
			CGRect rect = this->ios_window.window.frame;
			rect.origin.y += statusBarSize.height;
			rect.size.height -= statusBarSize.height;
			this->ios_window.window.rootViewController.view.frame = rect;
		}

		// TODO: this was setting the viewport, is something still needed here?
		// update_window_rect(
		// 		ruis::rect(
		// 				ruis::vec2(0),
		// 				ruis::vec2(
		// 						round(this->ios_window.window.frame.size.width * scale),
		// 						round((this->ios_window.window.frame.size.height - statusBarSize.height) * scale)
		// 					)
		// 			)
		// 	);

		this->ios_window.window.windowLevel = UIWindowLevelNormal;
	}
}

ruis::rect native_window::get_content_rect() const
{
	UIWindow* w = this->ios_window.window;

	float scale = [[UIScreen mainScreen] scale];

	if (this->is_fullscreen()) {
		return ruis::rect(
			ruis::vec2(0),
			ruis::vec2(
				round(w.frame.size.width * scale), //
				round(w.frame.size.height * scale)
			)
		);
	} else {
		CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;

		return ruis::rect(
			ruis::vec2(0),
			ruis::vec2(
				round(w.frame.size.width * scale), //
				round((w.frame.size.height - statusBarSize.height) * scale)
			)
		);
	}
}
