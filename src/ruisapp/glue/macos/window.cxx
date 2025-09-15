#include "window.hxx"

#include "key_code.map.hxx"

namespace{
void handle_mouse_button(NSEvent* e,//
	 bool is_down, ruis::mouse_button button, app_window& w){
	NSPoint win_pos = [e locationInWindow];
	using std::round;
	auto pos = round(ruis::vector2(win_pos.x, win_pos.y));
	
	// TODO:
	// pos.y() = w.dims().y() - pos.y();

	w.gui.send_mouse_button(is_down, //
		pos,
		button,
		0
		);

		// TODO: remove
	// handle_mouse_button(
	// 		ruisapp::application::inst(),
	// 		isDown,
	// 		ruis::vector2(pos.x(), ruisapp::application::inst().window_dims().y() - pos.y()),
	// 		button,
	// 		0
	// 	);
}
}

@implementation CocoaView

- (id)initWithFrame:(NSRect)rect
{
	utki::log_debug([&](auto&o){o << "CocoaView::initWithFrame(): enter" << std::endl;});
	self = [super initWithFrame:rect];
	if (!self) {
		return nil;
	}
	self->window = nullptr;
	self->ta = [[NSTrackingArea alloc] initWithRect:rect
											options:(NSTrackingActiveAlways | NSTrackingInVisibleRect |
													 NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved)
											  owner:self
										   userInfo:nil];
	[self addTrackingArea:self->ta];
	utki::log_debug([&](auto&o){o << "CocoaView::initWithFrame(): exit" << std::endl;});
	return self;
}

- (void)dealloc
{
	[self->ta release];
	[super dealloc];
}

- (void)mouseDown:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "left mouse button down!" << std::endl;});

	if(!self->window){
		return;
	}

	handle_mouse_button(e,//
		 true, ruis::mouse_button::left, *self->window);
}

- (void)mouseUp:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "left mouse button up!" << std::endl;});

	// TODO:
	// mouseButton(e, false, ruis::mouse_button::left);
}

- (void)rightMouseDown:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "right mouse button down!" << std::endl;});
	// TODO:
	// mouseButton(e, true, ruis::mouse_button::right);
}

- (void)rightMouseUp:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "right mouse button up!" << std::endl;});
	// TODO:
	// mouseButton(e, false, ruis::mouse_button::right);
}

- (void)otherMouseDown:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "middle mouse button down!" << std::endl;});
	// TODO:
	// mouseButton(e, true, ruis::mouse_button::middle);
}

- (void)otherMouseUp:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "middle mouse button up!" << std::endl;});
	// TODO:
	// mouseButton(e, false, ruis::mouse_button::middle);
}

- (void)scrollWheel:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "mouse wheel!" << std::endl;});

	if ([e hasPreciseScrollingDeltas] == NO) {
		// ruis::mouse_button button;
		// //		TRACE(<< "dy = " << float(dy) << std::endl)
		// if ([e scrollingDeltaY] < 0) {
		// 	button = ruis::mouse_button::wheel_down;
		// } else if ([e scrollingDeltaY] > 0) {
		// 	button = ruis::mouse_button::wheel_up;
		// } else if ([e scrollingDeltaX] < 0) {
		// 	button = ruis::mouse_button::wheel_left;
		// } else if ([e scrollingDeltaX] > 0) {
		// 	button = ruis::mouse_button::wheel_right;
		// } else {
		// 	return;
		// }
		//		TRACE(<< "button = " << unsigned(button) << std::endl)

		// TODO:
		// mouseButton(e, true, button);
		// mouseButton(e, false, button);
	} else {
		utki::log_debug([&](auto& o) {
			o << "mouse wheel: precise scrolling deltas, UNIMPLEMENTED!!!!!" << std::endl;
		});
	}
}

- (void)mouseMoved:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "mouse moved!" << std::endl;});
	// NSPoint pos = [e locationInWindow];
	//	TRACE(<< "x = " << pos.x << std::endl)
	// TODO:
	// using std::round;
	// macosx_HandleMouseMove(round(ruis::vector2(pos.x, pos.y)), 0);
}

- (void)mouseDragged:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "mouse left dragged!" << std::endl;});
	[self mouseMoved:e];
}

- (void)rightMouseDragged:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "mouse right dragged!" << std::endl;});
	[self mouseMoved:e];
}

- (void)otherMouseDragged:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "mouse middle dragged!" << std::endl;});
	[self mouseMoved:e];
}

- (void)mouseEntered:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "mouse enter!" << std::endl;});
	[[self window] setAcceptsMouseMovedEvents:YES];
	// TODO:
	// macosx_HandleMouseHover(true);
}

- (void)mouseExited:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "mouse exit!" << std::endl;});
	[[self window] setAcceptsMouseMovedEvents:NO];
	// TODO:
	// macosx_HandleMouseHover(false);
}

- (void)keyDown:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "key down!" << std::endl;});
	// std::uint8_t kc = [e keyCode];
	// ruis::key key = key_code_map[kc];

	// if ([e isARepeat] == YES) {
	// 	// TODO:
	// 	// macosx_HandleCharacterInput([e characters], key);
	// 	return;
	// }

	// TODO:
	// macosx_HandleKeyEvent(true, key);
	// macosx_HandleCharacterInput([e characters], key);
}

- (void)keyUp:(NSEvent*)e
{
	utki::log_debug([&](auto&o){o << "key up!" << std::endl;});
	// TODO:
	// std::uint8_t kc = [e keyCode];
	// macosx_HandleKeyEvent(false, keyCodeMap[kc]);
}

@end

@implementation CocoaWindow

- (id)initWithContentRect:(NSRect)contentRect
				styleMask:(NSUInteger)windowStyle
				  backing:(NSBackingStoreType)bufferingType
					defer:(BOOL)deferCreation
{
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if (!self) {
		return nil;
	}
	//	[self setLevel:NSFloatingWindowLevel];
	[self setLevel:NSNormalWindowLevel];

	self->v = [[CocoaView alloc] initWithFrame:[self frameRectForContentRect:contentRect]];
	[self setContentView:self->v];

	[self initStuff];

	[self setShowsResizeIndicator:YES];
	[self setMinSize:NSMakeSize(0, 0)];
	[self setMaxSize:NSMakeSize(1000000000, 1000000000)];
	[self setIgnoresMouseEvents:NO];

	return self;
}

- (void)initStuff
{
	[self makeFirstResponder:self->v];
	[self setDelegate:self];
	[self makeKeyWindow];
	[self makeMainWindow];
}

- (void)dealloc
{
	[self->v release];
	[super dealloc];
}

- (void)windowDidResize:(NSNotification*)n
{
	utki::log_debug([&](auto& o) {
		o << "window resize!!!!" << std::endl;
	});
	// TODO:
	// NSWindow* nsw = [n object];
	// NSRect frame = [nsw frame];
	// NSRect rect = [nsw contentRectForFrameRect:frame];
	// macosx_UpdateWindowRect(ruis::rect(0, 0, rect.size.width, rect.size.height));
}

- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
{
	return frameSize;
}

- (BOOL)windowShouldClose:(id)sender
{
	utki::log_debug([&](auto& o) {
		o << "window wants to close!!!!" << std::endl;
	});
	// TODO: call window close handler
	application::inst().quit();
	return NO;
}

- (BOOL)canBecomeKeyWindow
{
	// This is needed for window without title bar to be able to get key events
	return YES;
}

- (BOOL)canBecomeMainWindow
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (CocoaView*)view
{
	return self->v;
}

@end

void native_window::set_mouse_cursor_visible(bool visible)
{
	if (visible) {
		if (!this->mouse_cursor_currently_visible) {
			[NSCursor unhide];
		}
	} else {
		if (this->mouse_cursor_currently_visible) {
			[NSCursor hide];
		}
	}
	this->mouse_cursor_currently_visible = visible;
}
