#pragma once

#import <Cocoa/Cocoa.h>
#include <ruis/render/native_window.hpp>

namespace {
class app_window;
} // namespace

@interface CocoaView : NSView {
	app_window* window = nullptr;
	NSTrackingArea* ta;
}

- (id)initWithFrame:(NSRect)rect //
	associated_app_window:(app_window*)associated_app_window;
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
	CocoaView* v;
}

- (id)initWithContentRect:(NSRect)contentRect //
				styleMask:(NSUInteger)windowStyle
				  backing:(NSBackingStoreType)bufferingType
					defer:(BOOL)deferCreation
	associated_app_window:(app_window*)associated_app_window;
- (void)dealloc;

- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (BOOL)acceptsFirstResponder;

- (void)windowDidResize:(NSNotification*)n;
- (BOOL)windowShouldClose:(id)sender;
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize;

- (void)initStuff;

@end

namespace {
class native_window : public ruis::render::native_window
{
	struct cocoa_window_wrapper {
		const CocoaWindow* window;

		cocoa_window_wrapper(
			const ruisapp::window_parameters& window_params, //
			app_window* associated_app_window
		) :
			window([&]() {
				auto w = [[CocoaWindow alloc]
					  initWithContentRect:NSMakeRect(0, 0, window_params.dims.x(), window_params.dims.y())
								styleMask:(NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable |
										   NSWindowStyleMaskClosable | NSWindowStyleMaskTitled)
								  backing:NSBackingStoreBuffered
									defer:NO
					associated_app_window:associated_app_window];
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
		const NSOpenGLContext* context;

		opengl_context_wrapper(
			const ruisapp::window_parameters& window_params, //
			const NSOpenGLContext* shared_context
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

				auto c = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:shared_context];

				if (!c) {
					throw std::runtime_error(
						"opengl_context_wrapper::opengl_context_wrapper(): failed to create OpenGL context"
					);
				}

				return c;
			}())
		{
			// if there are no any GL contexts current, then set this one
			if ([NSOpenGLContext currentContext] == nil) {
				this->bind_rendering_context();
			}
			if (glewInit() != GLEW_OK) {
				throw std::runtime_error("GLEW initialization failed");
			}
		}

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

	bool mouse_cursor_currently_visible = true;

public:
	native_window(
		const utki::version_duplet& gl_version,
		const ruisapp::window_parameters& window_params,
		native_window* shared_gl_context_native_window
	) :
		cocoa_window(
			window_params, //
			associated_app_window
		),
		opengl_context(
			window_params, //
			shared_gl_context_native_window ? shared_gl_context_native_window->opengl_context.context
											: nullptr // no shared context
		)
	{
		[this->opengl_context.context setView:[this->cocoa_window.window contentView]];
	}

	void set_app_window(app_window* w)
	{
		utki::assert(!this->cocoa_window.window->v->window, SL);
		this->cocoa_window.window->v->window = w;
	}

	void bind_rendering_context() override
	{
		[this->opengl_context.context makeCurrentContext];
	}

	void set_mouse_cursor_visible(bool visible);
};
} // namespace