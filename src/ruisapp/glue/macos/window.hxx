#pragma once

#import <Cocoa/Cocoa.h>
#include <ruis/render/native_window.hpp>
#include <utki/debug.hpp>

#ifdef assert
#	undef assert
#endif

namespace {
class app_window;
} // namespace

@interface CocoaView : NSView {
@public
	app_window* window;
	NSTrackingArea* ta;
}

- (id)initWithFrame:(NSRect)rect;
- (void)dealloc;

- (void)mouseDown:(NSEvent*)e;
- (void)mouseUp:(NSEvent*)e;
- (void)rightMouseDown:(NSEvent*)e;
- (void)rightMouseUp:(NSEvent*)e;
- (void)otherMouseDown:(NSEvent*)e;
- (void)otherMouseUp:(NSEvent*)e;
- (void)scrollWheel:(NSEvent*)e;

- (void)mouseDragged:(NSEvent*)e;
- (void)rightMouseDragged:(NSEvent*)e;
- (void)otherMouseDragged:(NSEvent*)e;
- (void)mouseMoved:(NSEvent*)e;
- (void)mouseEntered:(NSEvent*)e;
- (void)mouseExited:(NSEvent*)e;

- (void)keyDown:(NSEvent*)e;
- (void)keyUp:(NSEvent*)e;

@end

@interface CocoaWindow : NSWindow <NSWindowDelegate> {
@public
	CocoaView* view;
}

- (id)initWithContentRect:(NSRect)contentRect //
				styleMask:(NSUInteger)windowStyle
				  backing:(NSBackingStoreType)bufferingType
					defer:(BOOL)deferCreation;
- (void)dealloc;

- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (BOOL)acceptsFirstResponder;

- (void)windowDidResize:(NSNotification*)n;
- (BOOL)windowShouldClose:(id)sender;
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize;

- (void)init_stuff;

@end

namespace {
class native_window : public ruis::render::native_window
{
	struct cocoa_window_wrapper {
		CocoaWindow* const window;

		cocoa_window_wrapper(const ruisapp::window_parameters& window_params) :
			window([&]() {
				auto w = [[CocoaWindow alloc]
					initWithContentRect:NSMakeRect(
											0, //
											0,
											window_params.dims.x(),
											window_params.dims.y()
										)
							  styleMask:(NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable |
										 NSWindowStyleMaskClosable | NSWindowStyleMaskTitled)
								backing:NSBackingStoreBuffered
								  defer:NO];
				if (!w) {
					throw std::runtime_error(
						"cocoa_window_wrapper::cocoa_window_wrapper(): failed to create cocoa window object"
					);
				}
				return w;
			}())
		{
			[this->window setTitle:[NSString stringWithUTF8String:window_params.title.c_str()]];

			if (window_params.visible) {
				[this->window makeKeyAndOrderFront:nil];
				[this->window orderFrontRegardless];
			}
		}

		cocoa_window_wrapper(const cocoa_window_wrapper&) = delete;
		cocoa_window_wrapper& operator=(const cocoa_window_wrapper&) = delete;

		cocoa_window_wrapper(cocoa_window_wrapper&&) = delete;
		cocoa_window_wrapper& operator=(cocoa_window_wrapper&&) = delete;

		~cocoa_window_wrapper()
		{
			[this->window release];
		}
	} cocoa_window;

	struct opengl_context_wrapper {
		NSOpenGLContext* const context;

		opengl_context_wrapper(
			const ruisapp::window_parameters& window_params, //
			NSOpenGLContext* shared_context
		) :
			context([&]() {
				std::vector<NSOpenGLPixelFormatAttribute> attributes;
				attributes.push_back(NSOpenGLPFAAccelerated);
				attributes.push_back(NSOpenGLPFAColorSize);
				attributes.push_back(24);
				if (window_params.buffers.get(ruisapp::buffer::depth)) {
					attributes.push_back(NSOpenGLPFADepthSize);
					attributes.push_back(16);
				}
				if (window_params.buffers.get(ruisapp::buffer::stencil)) {
					attributes.push_back(NSOpenGLPFAStencilSize);
					attributes.push_back(8);
				}
				attributes.push_back(NSOpenGLPFADoubleBuffer);
				attributes.push_back(NSOpenGLPFASupersample);
				attributes.push_back(0);

				NSOpenGLPixelFormat* pixel_format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes.data()];
				if (pixel_format == nil) {
					throw std::runtime_error(
						"opengl_context_wrapper::opengl_context_wrapper(): failed to create pixel format"
					);
				}

				utki::scope_exit pixel_format_scope_exit([&]() {
					[pixel_format release];
				});

				auto c = [[NSOpenGLContext alloc] initWithFormat:pixel_format shareContext:shared_context];

				if (!c) {
					throw std::runtime_error(
						"opengl_context_wrapper::opengl_context_wrapper(): failed to create OpenGL context"
					);
				}

				return c;
			}())
		{}

		opengl_context_wrapper(const opengl_context_wrapper&) = delete;
		opengl_context_wrapper& operator=(const opengl_context_wrapper&) = delete;

		opengl_context_wrapper(opengl_context_wrapper&&) = delete;
		opengl_context_wrapper& operator=(opengl_context_wrapper&&) = delete;

		~opengl_context_wrapper()
		{
			if ([NSOpenGLContext currentContext] == this->context) {
				[NSOpenGLContext clearCurrentContext];
			}
			[this->context release];
		}
	} opengl_context;

	r4::rectangle<int> before_fullscreen_window_rect{0, 0, 0, 0};

	bool mouse_cursor_currently_visible = true;

	ruis::vec2 cur_win_dims;

public:
	native_window(
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	) :
		cocoa_window(window_params),
		opengl_context(
			window_params, //
			shared_gl_context_native_window ? shared_gl_context_native_window->opengl_context.context
											: nullptr // no shared context
		),
		cur_win_dims(window_params.dims.to<ruis::real>())
	{
		[this->opengl_context.context setView:[this->cocoa_window.window contentView]];

		// if there are no any GL contexts current, then set this one
		if ([NSOpenGLContext currentContext] == nil) {
			[this->opengl_context.context makeCurrentContext];
		}
		if (glewInit() != GLEW_OK) {
			throw std::runtime_error("GLEW initialization failed");
		}
	}

	void set_app_window(app_window* w)
	{
		utki::assert(!this->cocoa_window.window->view->window, SL);
		this->cocoa_window.window->view->window = w;
	}

	void bind_rendering_context() override
	{
		[this->opengl_context.context makeCurrentContext];
	}

	void swap_frame_buffers() override
	{
		[this->opengl_context.context flushBuffer];
	}

	void set_mouse_cursor_visible(bool visible) override;

	bool is_mouse_cursor_visible() const noexcept
	{
		return this->mouse_cursor_currently_visible;
	}

	const ruis::vec2& dims()const noexcept{
		return this->cur_win_dims;
	}

	void resize(const ruis::vec2& dims){
		this->cur_win_dims = dims;

		// after resizing window we need to update the OpenGL context
		[this->opengl_context.context update];
	}

	void set_fullscreen_internal(bool enable)override;
};
} // namespace