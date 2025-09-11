#include "../../application.hpp"

#include <papki/fs_file.hpp>

#include <utki/destructable.hpp>

#include <ruis/util/util.hpp>
#include <ruis/gui.hpp>

#include <ruis/render/opengl/context.hpp>

#import <Cocoa/Cocoa.h>

using namespace ruisapp;

#include "../unix_common.hxx"

#include "application.hxx"


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
	// NSApplication* applicationObjectId;
	CocoaWindow* windowObjectId;
	NSOpenGLContext* openglContextId;

	// TODO: use atomic
	bool quitFlag = false;

	bool mouseCursorIsCurrentlyVisible = true;

	WindowWrapper(const window_parameters& wp);

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
		utki::log_debug([&](auto&o){o << "mouse wheel: precise scrolling deltas, UNIMPLEMENTED!!!!!" << std::endl;});
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
	utki::log_debug([&](auto&o){o << "window resize!!!!" << std::endl;});
	NSWindow* nsw = [n object];
	NSRect frame = [nsw frame];
	NSRect rect = [nsw contentRectForFrameRect:frame];
	macosx_UpdateWindowRect(ruis::rect(0, 0, rect.size.width, rect.size.height));
}

-(NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize{
	return frameSize;
}

-(BOOL)windowShouldClose:(id)sender{
	utki::log_debug([&](auto&o){o << "window wants to close!!!!" << std::endl;});
	application::inst().quit();
	return NO;
}

-(BOOL)canBecomeKeyWindow{return YES;} // This is needed for window without title bar to be able to get key events
-(BOOL)canBecomeMainWindow{return YES;}
-(BOOL)acceptsFirstResponder{return YES;}

-(CocoaView*)view{return self->v;}

@end

namespace{
WindowWrapper::WindowWrapper(const window_parameters& wp){
	utki::log_debug([&](auto&o){o << "WindowWrapper::WindowWrapper(): enter" << std::endl;});
	// this->applicationObjectId = [NSApplication sharedApplication];

	// if(!this->applicationObjectId){
	// 	throw std::runtime_error("WindowWrapper::WindowWrapper(): failed to create application object");
	// }

	// utki::scope_exit scopeExitApplication([this](){
	// 	[this->applicationObjectId release];
	// });

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
		if(wp.buffers.get(ruisapp::buffer::depth)){
			attributes.push_back(NSOpenGLPFADepthSize); attributes.push_back(16);
		}
		if(wp.buffers.get(ruisapp::buffer::stencil)){
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
	// scopeExitApplication.release();

	utki::log_debug([&](auto&o){o << "WindowWrapper::WindowWrapper(): exit" << std::endl;});
}
}

namespace{
WindowWrapper::~WindowWrapper()noexcept{
	[this->openglContextId release];
	[this->windowObjectId release];
	// [this->applicationObjectId release];
}
}

void application::quit()noexcept{
	auto& ww = get_impl(this->window_pimpl);
	ww.quitFlag = true;
}

int main(int argc, const char** argv){
	utki::log_debug([&](auto&o){o << "main(): enter" << std::endl;});
	auto application = ruisapp::application_factory::make_application(argc, argv);
	if(!application){
		// Not an error. The app just did not show any GUI to the user.
		return 0;
	}

	auto& app = *application;
	auto& glue = get_glue(app);

	utki::log_debug([&](auto&o){o << "main(): application instance created" << std::endl;});

	auto& ww = get_impl(get_window_pimpl(*app));

	// [ww.applicationObjectId activateIgnoringOtherApps:YES];

	[ww.windowObjectId makeKeyAndOrderFront:nil];

	[ww.windowObjectId orderFrontRegardless];

	// in order to get keyboard events we need to be foreground application
	{
		ProcessSerialNumber psn = {0, kCurrentProcess};
		OSStatus status = TransformProcessType(&psn, kProcessTransformToForegroundApplication);
		if(status != errSecSuccess){
			utki::assert(false, SL);
		}
	}

	do{
		// main loop cycle sequence as required by ruis:
		// - update updateables
		// - render
		// - wait for events and handle them
		uint32_t millis = ruisapp::inst().gui.update();

		glue.render();

		// wait for events
		NSEvent *event = [glue.macos_application.application
				nextEventMatchingMask:NSEventMaskAny
				untilDate:[NSDate dateWithTimeIntervalSinceNow:(double(millis) / std::milli::den)]
				inMode:NSDefaultRunLoopMode
				dequeue:YES
			];
		if(!event){
			continue;
		}

		// handle events
		do{
//			std::cout << "event: type = "<< [event type] << std::endl;
			switch([event type]){
				case NSEventTypeApplicationDefined:
					{
						auto data = [event data1];
						utki::assert(data, SL);
						// TODO: use static_cast?
						std::unique_ptr<std::function<void()>> m(reinterpret_cast<std::function<void()>*>(data));
						(*m)();
					}
					break;
				default:
					[glue.macos_application.application sendEvent:event];
					[glue.macos_application.application updateWindows];
					break;
			}

			event = [glue.macos_application.application
					nextEventMatchingMask:NSEventMaskAny
					untilDate:[NSDate distantPast]
					inMode:NSDefaultRunLoopMode
					dequeue:YES
				];
		}while(event && !glue.quit_flag.load());
	}while(!glue.quit_flag.load());

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

application::application(
	std::string name,
	const window_parameters& wp
) :
	name(name),
	window_pimpl(std::make_unique<WindowWrapper>(wp)),
	gui(utki::make_shared<ruis::context>(
		utki::make_shared<ruis::style_provider>(
			utki::make_shared<ruis::resource_loader>(
				utki::make_shared<ruis::render::renderer>(
					utki::make_shared<ruis::render::opengl::context>()
				)
			)
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
	utki::log_debug([&](auto&o){o << "application::application(): enter" << std::endl;});
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
