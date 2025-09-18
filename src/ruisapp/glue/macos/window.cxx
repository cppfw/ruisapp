#include "window.hxx"

#include "key_code.map.hxx"

namespace {
void handle_mouse_button(
	NSEvent* e, //
	bool is_down,
	ruis::mouse_button button,
	app_window& w
)
{
	NSPoint win_pos = [e locationInWindow];
	using std::round;
	auto pos = round(ruis::vector2(win_pos.x, win_pos.y));

	// TODO:
	// pos.y() = w.dims().y() - pos.y();

	utki::logcat_debug("mouse down pos = ", pos, '\n');

	w.gui.send_mouse_button(
		is_down, //
		pos,
		button,
		0
	);
}
} // namespace

namespace{
void handle_mouse_move(
	NSEvent* e, //
	app_window& w
)
{
	NSPoint win_pos = [e locationInWindow];

	using std::round;
	auto pos = round(ruis::vector2(win_pos.x, win_pos.y));

	// TODO:
	// pos.y() = w.dims().y() - pos.y();

	utki::logcat_debug("mouse move pos = ", pos, '\n');

	w.gui.send_mouse_move(
		pos, //
		0 // pointer id
	);
}
}

namespace{
void handle_mouse_hover(
	bool is_hovered, //
	app_window& w
)
{
	if(!w.ruis_native_window.get().is_mouse_cursor_visible()){
		if(is_hovered){
			[NSCursor hide];
		}else{
			[NSCursor unhide];
		}
	}

	utki::logcat_debug("window mouse hovered: ", is_hovered, '\n');

	w.gui.send_mouse_hover(
		is_hovered, //
		0 // pointer id
	);
}
}

namespace{
void handle_key_event(
	bool is_down, //
	ruis::key key_code,
	app_window& w
){
	w.gui.send_key(
		is_down,//
		key_code
	);
}
}

namespace{
void handle_character_input(
	NSEvent* e, //
	ruis::key key,
	app_window& w
){
	class macos_input_string_provider : public ruis::gui::input_string_provider{
		const NSString* nsStr;
	public:
		macos_input_string_provider(const NSString* nsStr = nullptr) :
				nsStr(nsStr)
		{}

		std::u32string get()const override{
			if(!this->nsStr){
				return std::u32string();
			}

			NSUInteger len = [this->nsStr length];

			std::u32string ret(len, 0);
			for(unsigned i = 0; i != len; ++i){
				ret[i] = [this->nsStr characterAtIndex:i];
			}

			return ret;
		}
	};

	const void* nsstring = [e characters];
	
	w.gui.send_character_input(
		macos_input_string_provider(static_cast<const NSString*>(nsstring)),//
		key
	);
}
}

@implementation CocoaView

- (id)initWithFrame:(NSRect)rect
{
	utki::log_debug([&](auto& o) {
		o << "CocoaView::initWithFrame(): enter" << std::endl;
	});
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
	utki::log_debug([&](auto& o) {
		o << "CocoaView::initWithFrame(): exit" << std::endl;
	});
	return self;
}

- (void)dealloc
{
	[self->ta release];
	[super dealloc];
}

- (void)mouseDown:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "left mouse button down!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	handle_mouse_button(
		e, //
		true,
		ruis::mouse_button::left,
		*self->window
	);
}

- (void)mouseUp:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "left mouse button up!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	handle_mouse_button(
		e, //
		false,
		ruis::mouse_button::left,
		*self->window
	);
}

- (void)rightMouseDown:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "right mouse button down!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	handle_mouse_button(
		e, //
		true,
		ruis::mouse_button::right,
		*self->window
	);
}

- (void)rightMouseUp:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "right mouse button up!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	handle_mouse_button(
		e, //
		false,
		ruis::mouse_button::right,
		*self->window
	);
}

- (void)otherMouseDown:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "middle mouse button down!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	handle_mouse_button(
		e, //
		true,
		ruis::mouse_button::middle,
		*self->window
	);
}

- (void)otherMouseUp:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "middle mouse button up!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	handle_mouse_button(
		e, //
		false,
		ruis::mouse_button::middle,
		*self->window
	);
}

- (void)scrollWheel:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "mouse wheel!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	if ([e hasPreciseScrollingDeltas] == NO) {
		ruis::mouse_button button;
		if ([e scrollingDeltaY] < 0) {
			button = ruis::mouse_button::wheel_down;
		} else if ([e scrollingDeltaY] > 0) {
			button = ruis::mouse_button::wheel_up;
		} else if ([e scrollingDeltaX] < 0) {
			button = ruis::mouse_button::wheel_left;
		} else if ([e scrollingDeltaX] > 0) {
			button = ruis::mouse_button::wheel_right;
		} else {
			return;
		}

		handle_mouse_button(
			e, //
			true,
			button,
			*self->window
		);

		handle_mouse_button(
			e, //
			false,
			button,
			*self->window
		);
	} else {
		utki::log_debug([&](auto& o) {
			o << "mouse wheel: precise scrolling deltas, UNIMPLEMENTED!!!!!" << std::endl;
		});
	}
}

- (void)mouseMoved:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "mouse moved!" << std::endl;
	});

	if (!self->window) {
		return;
	}
	
	handle_mouse_move(
		e, //
		*self->window
	);
}

- (void)mouseDragged:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "mouse left dragged!" << std::endl;
	});
	[self mouseMoved:e];
}

- (void)rightMouseDragged:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "mouse right dragged!" << std::endl;
	});
	[self mouseMoved:e];
}

- (void)otherMouseDragged:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "mouse middle dragged!" << std::endl;
	});
	[self mouseMoved:e];
}

- (void)mouseEntered:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "mouse enter!" << std::endl;
	});
	[[self window] setAcceptsMouseMovedEvents:YES];

	if (!self->window) {
		return;
	}

	handle_mouse_hover(true,//
		*self->window);
}

- (void)mouseExited:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "mouse exit!" << std::endl;
	});
	[[self window] setAcceptsMouseMovedEvents:NO];

	if (!self->window) {
		return;
	}

	handle_mouse_hover(false,//
		*self->window);
}

- (void)keyDown:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "key down!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	std::uint8_t kc = [e keyCode];
	ruis::key key = key_code_map[kc];

	if ([e isARepeat] == YES) {
		handle_character_input(e,//
			key,
		*self->window);
		return;
	}

	handle_key_event(true, //
		key,
	*self->window);

	handle_character_input(e,//
			key,
		*self->window);
}

- (void)keyUp:(NSEvent*)e
{
	utki::log_debug([&](auto& o) {
		o << "key up!" << std::endl;
	});

	if (!self->window) {
		return;
	}

	std::uint8_t kc = [e keyCode];
	ruis::key key = key_code_map[kc];

	handle_key_event(false, //
		key,
	*self->window);
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

	// TODO: is remove commented code?
	//	[self setLevel:NSFloatingWindowLevel];

	[self setLevel:NSNormalWindowLevel];

	self->view = [[CocoaView alloc] initWithFrame:[self frameRectForContentRect:contentRect]];
	[self setContentView: self->view];

	[self initStuff];

	[self setShowsResizeIndicator:YES];
	[self setMinSize:NSMakeSize(0, 0)];
	[self setMaxSize:NSMakeSize(1000000000, 1000000000)];
	[self setIgnoresMouseEvents:NO];

	return self;
}

- (void)initStuff
{
	[self makeFirstResponder:self->view];
	[self setDelegate:self];
	[self makeKeyWindow];
	[self makeMainWindow];
}

- (void)dealloc
{
	[self->view release];
	[super dealloc];
}

- (void)windowDidResize:(NSNotification*)n
{
	utki::log_debug([&](auto& o) {
		o << "window resize!!!!" << std::endl;
	});

	auto* w = self->view->window;

	if(!w){
		return;
	}

//	auto& natwin = w->ruis_native_window.get();

	NSWindow* nsw = [n object];
	NSRect frame = [nsw frame];
	NSRect rect = [nsw contentRectForFrameRect:frame];

	ruis::rect new_win_rect(0, 0, rect.size.width, rect.size.height);

	// TODO:
	// 	[ww.openglContextId update]; // after resizing window we need to update OpenGL context
	// 	update_window_rect(ruisapp::application::inst(), r);
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

	auto* w = self->view->window;

	if(!w){
		// TODO: what does this NO mean?
		return NO;
	}

	auto& natwin = w->ruis_native_window.get();

	if(natwin.close_handler){
		natwin.close_handler();
	}

	// TODO: what does this NO mean?
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
