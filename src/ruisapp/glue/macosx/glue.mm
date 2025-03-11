#include "../../application.hpp"

#include <papki/fs_file.hpp>

#include <utki/destructable.hpp>

#include <ruis/util/util.hpp>
#include <ruis/gui.hpp>

#include <ruis/render/opengl/context.hpp>

#import <Cocoa/Cocoa.h>

using namespace ruisapp;

#include "../unix_common.cxx"
#include "../friend_accessors.cxx"

@interface CocoaView : NSView{
	NSTrackingArea* ta;
}

-(id)initWithFrame:(NSRect)rect;
-(void)dealloc;

-(void)mouseDown: (NSEvent*)e;
-(void)mouseUp: (NSEvent*)e;
-(void)rightMouseDown: (NSEvent*)e;
-(void)rightMouseUp: (NSEvent*)e;
-(void)otherMouseDown: (NSEvent*)e;
-(void)otherMouseUp: (NSEvent*)e;
-(void)scrollWheel:(NSEvent*)e;

-(void)mouseDragged: (NSEvent*)e;
-(void)rightMouseDragged: (NSEvent*)e;
-(void)otherMouseDragged: (NSEvent*)e;
-(void)mouseMoved: (NSEvent*)e;
-(void)mouseEntered: (NSEvent*)e;
-(void)mouseExited: (NSEvent*)e;

-(void)keyDown:(NSEvent*)e;
-(void)keyUp:(NSEvent*)e;

@end

@interface CocoaWindow : NSWindow <NSWindowDelegate>{
	CocoaView* v;
}

-(id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation;
-(void)dealloc;

-(BOOL)canBecomeKeyWindow;
-(BOOL)canBecomeMainWindow;
-(BOOL)acceptsFirstResponder;

-(void)windowDidResize:(NSNotification*)n;
-(BOOL)windowShouldClose:(id)sender;
-(NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize;

-(void)initStuff;

@end

namespace{
struct WindowWrapper : public utki::destructable{
	NSApplication* applicationObjectId;
	CocoaWindow* windowObjectId;
	NSOpenGLContext* openglContextId;

	// TODO: use atomic
	bool quitFlag = false;

	bool mouseCursorIsCurrentlyVisible = true;

	WindowWrapper(const window_params& wp);

	~WindowWrapper()noexcept;
};

WindowWrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl){
	ASSERT(dynamic_cast<WindowWrapper*>(pimpl.get()))
	return static_cast<WindowWrapper&>(*pimpl);
}
}

namespace{
void mouseButton(NSEvent* e, bool isDown, ruis::mouse_button button){
	NSPoint winPos = [e locationInWindow];
	using std::round;
	auto pos = round(ruis::vector2(winPos.x, winPos.y));
	handle_mouse_button(
			ruisapp::application::inst(),
			isDown,
			ruis::vector2(pos.x(), ruisapp::application::inst().window_dims().y() - pos.y()),
			button,
			0
		);
}

void macosx_HandleMouseMove(const ruis::vector2& pos, unsigned id){
//	TRACE(<< "Macosx_HandleMouseMove(): pos = " << pos << std::endl)
	handle_mouse_move(
			ruisapp::application::inst(),
			ruis::vector2(pos.x(), ruisapp::application::inst().window_dims().y() - pos.y()),
			id
		);
}

void macosx_HandleMouseHover(bool isHovered){
	auto& ww = get_impl(get_window_pimpl(ruisapp::application::inst()));
	if(!ww.mouseCursorIsCurrentlyVisible){
		if(isHovered){
			[NSCursor hide];
		}else if(!isHovered){
			[NSCursor unhide];
		}
	}

	handle_mouse_hover(ruisapp::application::inst(), isHovered, 0);
}

void macosx_HandleKeyEvent(bool isDown, ruis::key keyCode){
	handle_key_event(ruisapp::application::inst(), isDown, keyCode);
}

class macosx_input_string_provider : public ruis::gui::input_string_provider{
	const NSString* nsStr;
public:
	macosx_input_string_provider(const NSString* nsStr = nullptr) :
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

void macosx_HandleCharacterInput(const void* nsstring, ruis::key key){
	handle_character_input(ruisapp::application::inst(), macosx_input_string_provider(reinterpret_cast<const NSString*>(nsstring)), key);
}

void macosx_UpdateWindowRect(const ruis::rect& r){
	auto& ww = get_impl(get_window_pimpl(ruisapp::application::inst()));
	[ww.openglContextId update]; // after resizing window we need to update OpenGL context
	update_window_rect(ruisapp::application::inst(), r);
}

const std::array<ruis::key, std::uint8_t(-1) + 1> keyCodeMap = {{
	ruis::key::a, // 0
	ruis::key::s,
	ruis::key::d,
	ruis::key::f,
	ruis::key::h,
	ruis::key::g, // 5
	ruis::key::z,
	ruis::key::x,
	ruis::key::c,
	ruis::key::v,
	ruis::key::unknown, // 0x0A
	ruis::key::b,
	ruis::key::q,
	ruis::key::w,
	ruis::key::e,
	ruis::key::r, // 15
	ruis::key::y,
	ruis::key::t,
	ruis::key::one,
	ruis::key::two,
	ruis::key::three, // 20
	ruis::key::four,
	ruis::key::six,
	ruis::key::five, // 0x17
	ruis::key::equals,
	ruis::key::nine, // 25
	ruis::key::seven,
	ruis::key::minus,
	ruis::key::eight,
	ruis::key::zero,
	ruis::key::right_square_bracket, // 30
	ruis::key::o,
	ruis::key::u,
	ruis::key::left_square_bracket,
	ruis::key::i,
	ruis::key::p, // 35
	ruis::key::enter, // 0x24
	ruis::key::l,
	ruis::key::j,
	ruis::key::apostrophe,
	ruis::key::k, // 40
	ruis::key::semicolon,
	ruis::key::backslash,
	ruis::key::comma,
	ruis::key::slash,
	ruis::key::n, // 0x2D, 45
	ruis::key::m,
	ruis::key::period,
	ruis::key::tabulator, // 0x30
	ruis::key::space, // 0x31
	ruis::key::grave, // 50
	ruis::key::backspace, // 0x33
	ruis::key::unknown, // 0x34
	ruis::key::escape, // 0x35
	ruis::key::unknown, // 0x36
	ruis::key::left_command, // Command, 0x37, 55
	ruis::key::left_shift, // 0x38
	ruis::key::capslock, // 0x39
	ruis::key::unknown, // Option, 0x3A
	ruis::key::left_control, // 0x3B
	ruis::key::right_shift, // 0x3C, 60
	ruis::key::unknown, // RightOption, 0x3D
	ruis::key::right_control, // 0x3E
	ruis::key::function, // 0x3F
	ruis::key::f17, // 0x40
	ruis::key::unknown, // KeypadDecimal, 0x41, 65
	ruis::key::unknown, // 0x42
	ruis::key::unknown, // KeypadMultiplym 0x43
	ruis::key::unknown, // 0x44
	ruis::key::unknown, // KeypadPlus, 0x45
	ruis::key::unknown, // 0x46, 70
	ruis::key::unknown, // KeypadClear, 0x47
	ruis::key::unknown, // VolumeUp, 0x48
	ruis::key::unknown, // VolumeDown, 0x49
	ruis::key::unknown, // Mute, 0x4A
	ruis::key::unknown, // KeypadDivide, 0x4B, 75
	ruis::key::unknown, // KeypadEnter, 0x4C
	ruis::key::unknown, // 0x4D
	ruis::key::unknown, // KeypadMinus
	ruis::key::f18, // 0x4F
	ruis::key::f19, // 0x50, 80
	ruis::key::unknown, // KeypadEquals, 0x51
	ruis::key::unknown, // Keypad0
	ruis::key::unknown, // Keypad1
	ruis::key::unknown, // Keypad2
	ruis::key::unknown, // Keypad3, 85
	ruis::key::unknown, // Keypad4
	ruis::key::unknown, // Keypad5
	ruis::key::unknown, // Keypad6
	ruis::key::unknown, // Keypad7, 0x59
	ruis::key::f20, // 0x5A, 90
	ruis::key::unknown, // Keypad8, 0x5B
	ruis::key::unknown, // Keypad9, 0x5C
	ruis::key::unknown, // 0x5D
	ruis::key::unknown, // 0x5E
	ruis::key::unknown, // 0x5F, 95
	ruis::key::f5, // 0x60
	ruis::key::f6, // 0x61
	ruis::key::f7, // 0x62
	ruis::key::f3, // 0x63
	ruis::key::f8, // 0x64, 100
	ruis::key::f9, // 0x65
	ruis::key::unknown, // 0x66
	ruis::key::f11, // 0x67
	ruis::key::unknown, // 0x68
	ruis::key::f13, // 0x69
	ruis::key::f16, // 0x6A
	ruis::key::f14, // 0x6B
	ruis::key::unknown, // 0x6C
	ruis::key::f10, // 0x6D
	ruis::key::unknown, // 0x6E
	ruis::key::f12, // 0x6F
	ruis::key::unknown, // 0x70
	ruis::key::f15, // 0x71
	ruis::key::unknown, // Help, 0x72
	ruis::key::home, // 0x73
	ruis::key::page_up, // 0x74
	ruis::key::deletion, // 0x75
	ruis::key::f4, // 0x76
	ruis::key::end, // 0x77
	ruis::key::f2, // 0x78
	ruis::key::page_down, // 0x79
	ruis::key::f1, // 0x7A
	ruis::key::arrow_left, // 0x7B
	ruis::key::arrow_right, // 0x7C
	ruis::key::arrow_down, // 0x7D
	ruis::key::arrow_up, // 0x7E
	ruis::key::unknown, // 0x7F
	ruis::key::unknown, // 0x80
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown, // 0x90
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown, // 0xA0
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown, // 0xB0
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown, // 0xC0
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown, // 0xD0
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown, // 0xE0
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown, // 0xF0
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown,
	ruis::key::unknown // 0xFF
}};
}

@implementation CocoaView

-(id)initWithFrame:(NSRect)rect{
	self = [super initWithFrame:rect];
	if(!self){
		return nil;
	}
	self->ta = [[NSTrackingArea alloc]
			initWithRect: rect
			options: (NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved)
			owner: self
			userInfo: nil
		];
	[self addTrackingArea:self->ta];
	return self;
}

-(void)dealloc{
	[self->ta release];
	[super dealloc];
}

-(void)mouseDown: (NSEvent*)e{
//	TRACE(<< "left down!!!!!" << std::endl)
	mouseButton(e, true, ruis::mouse_button::left);
}

-(void)mouseUp: (NSEvent*)e{
//	TRACE(<< "left up!!!!!" << std::endl)
	mouseButton(e, false, ruis::mouse_button::left);
}

-(void)rightMouseDown: (NSEvent*)e{
//	TRACE(<< "right down!!!!!" << std::endl)
	mouseButton(e, true, ruis::mouse_button::right);
}

-(void)rightMouseUp: (NSEvent*)e{
//	TRACE(<< "right up!!!!!" << std::endl)
	mouseButton(e, false, ruis::mouse_button::right);
}

-(void)otherMouseDown: (NSEvent*)e{
//	TRACE(<< "middle down!!!!!" << std::endl)
	mouseButton(e, true, ruis::mouse_button::middle);
}

-(void)otherMouseUp: (NSEvent*)e{
//	TRACE(<< "middle up!!!!!" << std::endl)
	mouseButton(e, false, ruis::mouse_button::middle);
}

-(void)scrollWheel: (NSEvent*)e{
//	TRACE(<< "mouse wheel!!!!!" << std::endl)

	if([e hasPreciseScrollingDeltas] == NO){
		ruis::mouse_button button;
//		TRACE(<< "dy = " << float(dy) << std::endl)
		if([e scrollingDeltaY] < 0){
			button = ruis::mouse_button::wheel_down;
		}else if([e scrollingDeltaY] > 0){
			button = ruis::mouse_button::wheel_up;
		}else if([e scrollingDeltaX] < 0){
			button = ruis::mouse_button::wheel_left;
		}else if([e scrollingDeltaX] > 0){
			button = ruis::mouse_button::wheel_right;
		}else{
			return;
		}
//		TRACE(<< "button = " << unsigned(button) << std::endl)

		mouseButton(e, true, button);
		mouseButton(e, false, button);
	}else{
		LOG([&](auto&o){o << "mouse wheel: precise scrolling deltas, UNIMPLEMENTED!!!!!" << std::endl;})
	}
}

-(void)mouseMoved: (NSEvent*)e{
//	TRACE(<< "mouseMoved event!!!!!" << std::endl)
	NSPoint pos = [e locationInWindow];
//	TRACE(<< "x = " << pos.x << std::endl)
	using std::round;
	macosx_HandleMouseMove(
			round(ruis::vector2(pos.x, pos.y)),
			0
		);
}

-(void)mouseDragged: (NSEvent*)e{
	[self mouseMoved:e];
}

-(void)rightMouseDragged: (NSEvent*)e{
	[self mouseMoved:e];
}

-(void)otherMouseDragged: (NSEvent*)e{
	[self mouseMoved:e];
}

-(void)mouseEntered: (NSEvent*)e{
//	TRACE(<< "mouseEntered event!!!!!" << std::endl)
	[[self window] setAcceptsMouseMovedEvents:YES];
	macosx_HandleMouseHover(true);
}

-(void)mouseExited: (NSEvent*)e{
//	TRACE(<< "mouseExited event!!!!!" << std::endl)
	[[self window] setAcceptsMouseMovedEvents:NO];
	macosx_HandleMouseHover(false);
}

-(void)keyDown:(NSEvent*)e{
//	TRACE(<< "keyDown event!!!!!" << std::endl)
	std::uint8_t kc = [e keyCode];
	ruis::key key = keyCodeMap[kc];

	if([e isARepeat] == YES){
		macosx_HandleCharacterInput([e characters], key);
		return;
	}

	macosx_HandleKeyEvent(true, key);

	macosx_HandleCharacterInput([e characters], key);
}

-(void)keyUp:(NSEvent*)e{
//	TRACE(<< "keyUp event!!!!!" << std::endl)
	std::uint8_t kc = [e keyCode];
	macosx_HandleKeyEvent(false, keyCodeMap[kc]);
}

@end

@implementation CocoaWindow

-(id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation{
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if(!self){
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

-(void)initStuff{
	[self makeFirstResponder:self->v];
	[self setDelegate:self];
	[self makeKeyWindow];
	[self makeMainWindow];
}

-(void)dealloc{
	[self->v release];
	[super dealloc];
}

-(void)windowDidResize:(NSNotification*)n{
	LOG([&](auto&o){o << "window resize!!!!" << std::endl;})
	NSWindow* nsw = [n object];
	NSRect frame = [nsw frame];
	NSRect rect = [nsw contentRectForFrameRect:frame];
	macosx_UpdateWindowRect(ruis::rect(0, 0, rect.size.width, rect.size.height));
}

-(NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize{
	return frameSize;
}

-(BOOL)windowShouldClose:(id)sender{
	LOG([&](auto&o){o << "window wants to close!!!!" << std::endl;})
	application::inst().quit();
	return NO;
}

-(BOOL)canBecomeKeyWindow{return YES;} // This is needed for window without title bar to be able to get key events
-(BOOL)canBecomeMainWindow{return YES;}
-(BOOL)acceptsFirstResponder{return YES;}

-(CocoaView*)view{return self->v;}

@end

namespace{
WindowWrapper::WindowWrapper(const window_params& wp){
	LOG([&](auto&o){o << "WindowWrapper::WindowWrapper(): enter" << std::endl;})
	this->applicationObjectId = [NSApplication sharedApplication];

	if(!this->applicationObjectId){
		throw std::runtime_error("WindowWrapper::WindowWrapper(): failed to create application object");
	}

	utki::scope_exit scopeExitApplication([this](){
		[this->applicationObjectId release];
	});

	this->windowObjectId = [[CocoaWindow alloc]
		initWithContentRect:NSMakeRect(0, 0, wp.dims.x(), wp.dims.y())
		styleMask:(NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskTitled)
		backing:NSBackingStoreBuffered
		defer:NO
	];

	if(!this->windowObjectId){
		throw std::runtime_error("WindowWrapper::WindowWrapper(): failed to create Window object");
	}

	utki::scope_exit scopeExitWindow([this](){
		[this->windowObjectId release];
	});

	[this->windowObjectId setTitle:[NSString stringWithUTF8String:wp.title.c_str()]];

	{
		std::vector<NSOpenGLPixelFormatAttribute> attributes;
		attributes.push_back(NSOpenGLPFAAccelerated);
		attributes.push_back(NSOpenGLPFAColorSize); attributes.push_back(24);
		if(wp.buffers.get(window_params::buffer::depth)){
			attributes.push_back(NSOpenGLPFADepthSize); attributes.push_back(16);
		}
		if(wp.buffers.get(window_params::buffer::stencil)){
			attributes.push_back(NSOpenGLPFAStencilSize); attributes.push_back(8);
		}
		attributes.push_back(NSOpenGLPFADoubleBuffer);
		attributes.push_back(NSOpenGLPFASupersample);
		attributes.push_back(0);

		NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes.data()];
		if(pixelFormat == nil){
			throw std::runtime_error("WindowWrapper::WindowWrapper(): failed to create pixel format");
		}

		this->openglContextId = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
		[pixelFormat release];

		if(!this->openglContextId){
			throw std::runtime_error("WindowWrapper::WindowWrapper(): failed to create OpenGL context");
		}
	}

	utki::scope_exit scopeExitOpenGLContext([this](){
		[this->openglContextId release];
	});

	[this->openglContextId setView:[this->windowObjectId contentView]];
	[this->openglContextId makeCurrentContext];

	if(glewInit() != GLEW_OK){
		throw std::runtime_error("GLEW initialization failed");
	}

	scopeExitOpenGLContext.release();
	scopeExitWindow.release();
	scopeExitApplication.release();

	LOG([&](auto&o){o << "WindowWrapper::WindowWrapper(): exit" << std::endl;})
}
}

namespace{
WindowWrapper::~WindowWrapper()noexcept{
	[this->openglContextId release];
	[this->windowObjectId release];
	[this->applicationObjectId release];
}
}

void application::quit()noexcept{
	auto& ww = get_impl(this->window_pimpl);
	ww.quitFlag = true;
}

int main(int argc, const char** argv){
	LOG([&](auto&o){o << "main(): enter" << std::endl;})
	auto app = create_app_unix(argc, argv);
	if(!app){
		// Not an error. The app just did not show any GUI to the user.
		return 0;
	}

	LOG([&](auto&o){o << "main(): app created" << std::endl;})

	auto& ww = get_impl(get_window_pimpl(*app));

	[ww.applicationObjectId activateIgnoringOtherApps:YES];

	[ww.windowObjectId makeKeyAndOrderFront:nil];

	[ww.windowObjectId orderFrontRegardless];

	//in order to get keyboard events we need to be foreground application
	{
		ProcessSerialNumber psn = {0, kCurrentProcess};
		OSStatus status = TransformProcessType(&psn, kProcessTransformToForegroundApplication);
		if(status != errSecSuccess){
			ASSERT(false)
		}
	}

	do{
		// sequence:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle
		uint32_t millis = ruisapp::inst().gui.update();
		render(ruisapp::inst());
		NSEvent *event = [ww.applicationObjectId
				nextEventMatchingMask:NSEventMaskAny
				untilDate:[NSDate dateWithTimeIntervalSinceNow:(double(millis) / 1000.0)]
				inMode:NSDefaultRunLoopMode
				dequeue:YES
			];

		if(!event){
			continue;
		}

		do{
//			TRACE_ALWAYS(<< "Event: type = "<< [event type] << std::endl)
			switch([event type]){
				case NSEventTypeApplicationDefined:
					{
						std::unique_ptr<std::function<void()>> m(reinterpret_cast<std::function<void()>*>([event data1]));
						(*m)();
					}
					break;
				default:
					[ww.applicationObjectId sendEvent:event];
					[ww.applicationObjectId updateWindows];
					break;
			}

			event = [ww.applicationObjectId
					nextEventMatchingMask:NSEventMaskAny
					untilDate:[NSDate distantPast]
					inMode:NSDefaultRunLoopMode
					dequeue:YES
				];
		}while(event && !ww.quitFlag);
	}while(!ww.quitFlag);

	return 0;
}

namespace{
ruis::real getDotsPerInch(){
	NSScreen *screen = [NSScreen mainScreen];
	NSDictionary *description = [screen deviceDescription];
	NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
	CGSize displayPhysicalSize = CGDisplayScreenSize(
			[[description objectForKey:@"NSScreenNumber"] unsignedIntValue]
		);

	ruis::real value = ruis::real(((displayPixelSize.width * 10.0f / displayPhysicalSize.width) +
			(displayPixelSize.height * 10.0f / displayPhysicalSize.height)) / 2.0f);
	value *= 2.54f;
	return value;
}
}

namespace{
ruis::real getDotsPerPt(){
	NSScreen *screen = [NSScreen mainScreen];
	NSDictionary *description = [screen deviceDescription];
	NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
	CGSize displayPhysicalSize = CGDisplayScreenSize(
			[[description objectForKey:@"NSScreenNumber"] unsignedIntValue]
		);

	r4::vector2<unsigned> resolution(displayPixelSize.width, displayPixelSize.height);
	r4::vector2<unsigned> screenSizeMm(displayPhysicalSize.width, displayPhysicalSize.height);

	return application::get_pixels_per_pp(resolution, screenSizeMm);
}
}

application::application(std::string name, const window_params& wp) :
		name(name),
		window_pimpl(std::make_unique<WindowWrapper>(wp)),
		gui(utki::make_shared<ruis::context>(
			utki::make_shared<ruis::render::renderer>(
				utki::make_shared<ruis::render::opengl::context>()
			),
			utki::make_shared<ruis::updater>(),
			ruis::context::parameters{
				.post_to_ui_thread_function = [this](std::function<void()> a){
					auto& ww = get_impl(get_window_pimpl(*this));

					NSEvent* e = [NSEvent
							otherEventWithType: NSEventTypeApplicationDefined
							location: NSMakePoint(0, 0)
							modifierFlags:0
							timestamp:0
							windowNumber:0
							context: nil
							subtype: 0
							data1: reinterpret_cast<NSInteger>(new std::function<void()>(std::move(a)))
							data2: 0
						];

					[ww.applicationObjectId postEvent:e atStart:NO];
				},
				.set_mouse_cursor_function = [](ruis::mouse_cursor c){
					// TODO:
				},
				.units = ruis::units(
					getDotsPerInch(), //
					getDotsPerPt()
				)
			}
		)),
		directory(get_application_directories(this->name))
{
	LOG([&](auto&o){o << "application::application(): enter" << std::endl;})
	this->update_window_rect(
			ruis::rect(
					0,
					0,
					ruis::real(wp.dims.x()),
					ruis::real(wp.dims.y())
				)
		);
}

void application::swap_frame_buffers(){
	auto& ww = get_impl(this->window_pimpl);
	[ww.openglContextId flushBuffer];
}

void application::set_fullscreen(bool enable){
	if(enable == this->is_fullscreen()){
		return;
	}

	auto& ww = get_impl(this->window_pimpl);

	if(enable){
		// save old window size
		NSRect rect = [ww.windowObjectId frame];
		this->before_fullscreen_window_rect.p.x() = rect.origin.x;
		this->before_fullscreen_window_rect.p.y() = rect.origin.y;
		this->before_fullscreen_window_rect.d.x() = rect.size.width;
		this->before_fullscreen_window_rect.d.y() = rect.size.height;

		[ww.windowObjectId setStyleMask:([ww.windowObjectId styleMask] & (~(NSWindowStyleMaskTitled | NSWindowStyleMaskResizable)))];

		[ww.windowObjectId setFrame:[[NSScreen mainScreen] frame] display:YES animate:NO];
		[ww.windowObjectId setLevel:NSScreenSaverWindowLevel];
	}else{
		[ww.windowObjectId setStyleMask:([ww.windowObjectId styleMask] | NSWindowStyleMaskTitled | NSWindowStyleMaskResizable)];

		NSRect oldFrame;
		oldFrame.origin.x = this->before_fullscreen_window_rect.p.x();
		oldFrame.origin.y = this->before_fullscreen_window_rect.p.y();
		oldFrame.size.width = this->before_fullscreen_window_rect.d.x();
		oldFrame.size.height = this->before_fullscreen_window_rect.d.y();

		[ww.windowObjectId setFrame:oldFrame display:YES animate:NO];
		[ww.windowObjectId setLevel:NSNormalWindowLevel];
	}

	[ww.windowObjectId initStuff];

	this->is_fullscreen_v = enable;
}

void application::set_mouse_cursor_visible(bool visible){
	auto& ww = get_impl(this->window_pimpl);
	if(visible){
		if(!ww.mouseCursorIsCurrentlyVisible){
			[NSCursor unhide];
			ww.mouseCursorIsCurrentlyVisible = true;
		}
	}else{
		if(ww.mouseCursorIsCurrentlyVisible){
			[NSCursor hide];
			ww.mouseCursorIsCurrentlyVisible = false;
		}
	}
}
