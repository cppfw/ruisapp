#include "../../application.hpp"

#include <papki/fs_file.hpp>
#include <papki/root_dir_file.hpp>

#include <sstream>

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#include <ruis/render/opengles/renderer.hpp>

using namespace ruisapp;

#include "../unix_common.cxx"
#include "../friend_accessors.cxx"

@interface AppDelegate : UIResponder <UIApplicationDelegate>{
	application* app;
}

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	self->app = create_app_unix(0, nullptr).release();

	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
	ASSERT(self->app)
	delete self->app;
}

@end

int main(int argc, char * argv[]){
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	int retVal = UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	[pool release];
	return retVal;
}

@interface ViewController : GLKViewController{

}

@property (strong, nonatomic) EAGLContext *context;

@end

namespace{
	window_params windowParams(0);

	struct WindowWrapper : public utki::Unique{
		UIWindow *window;

		WindowWrapper(const window_params& wp){
			windowParams = wp;

			this->window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

			if(!this->window){
				throw ruis::Exc("failed to create a UIWindow");
			}

			utki::ScopeExit scopeExitWindow([this](){
				[this->window release];
			});


			this->window.screen = [UIScreen mainScreen];

			this->window.backgroundColor = [UIColor redColor];
			this->window.rootViewController = [[ViewController alloc] init];

			[this->window makeKeyAndVisible];

			scopeExitWindow.reset();
		}
		~WindowWrapper()noexcept{
			[this->window release];
		}
	};

	WindowWrapper& get_impl(const std::unique_ptr<utki::Unique>& pimpl){
		ASSERT(pimpl)
		ASSERT(dynamic_cast<WindowWrapper*>(pimpl.get()))
		return static_cast<WindowWrapper&>(*pimpl);
	}
}

@implementation ViewController

- (void)viewDidLoad{
	[super viewDidLoad];

	self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
	if (self.context == nil) {
		self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	}

	if (!self.context) {
		NSLog(@"Failed to create ES context");
	}

	GLKView *view = (GLKView *)self.view;
	view.context = self.context;
	view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;

	{
		const window_params& wp = windowParams;
		if(wp.buffers.get(window_params::buffer_type::depth)){
			view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
		}else{
			view.drawableDepthFormat = GLKViewDrawableDepthFormatNone;
		}
		if(wp.buffers.get(window_params::buffer_type::stencil)){
			view.drawableStencilFormat = GLKViewDrawableStencilFormat8;
		}else{
			view.drawableStencilFormat = GLKViewDrawableStencilFormatNone;
		}
	}

	[EAGLContext setCurrentContext:self.context];

	view.multipleTouchEnabled = YES;

	if([self respondsToSelector:@selector(edgesForExtendedLayout)]){
		self.edgesForExtendedLayout = UIRectEdgeNone;
	}
}

- (void)dealloc{
	[super dealloc];

	if ([EAGLContext currentContext] == self.context) {
		[EAGLContext setCurrentContext:nil];
	}
	[self.context release];
}

- (void)didReceiveMemoryWarning{
	// Dispose of any resources that can be recreated.
}

- (void)update{
	//TODO: adapt to nothing to update, lower frame rate
	ruisapp::inst().gui.update();
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect{
	render(ruisapp::inst());
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event{
	float scale = [UIScreen mainScreen].scale;

	for(UITouch * touch in touches ){
		CGPoint p = [touch locationInView:self.view ];

//		TRACE(<< "touch began = " << ruis::Vec2r(p.x * scale, p.y * scale).rounded() << std::endl)
		handle_mouse_button(
				ruisapp::inst(),
				true,
				ruis::Vec2r(p.x * scale, p.y * scale).rounded(),
				ruis::MouseButton_e::LEFT,
				0 //TODO: id
			);
	}
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event{
	float scale = [UIScreen mainScreen].scale;

	for(UITouch * touch in touches ){
		CGPoint p = [touch locationInView:self.view ];

//		TRACE(<< "touch moved = " << ruis::Vec2r(p.x * scale, p.y * scale).rounded() << std::endl)
		handle_mouse_move(
				ruisapp::inst(),
				ruis::Vec2r(p.x * scale, p.y * scale).rounded(),
				0 //TODO: id
			);
	}
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event{
	float scale = [UIScreen mainScreen].scale;

	for(UITouch * touch in touches ){
		CGPoint p = [touch locationInView:self.view ];

//		TRACE(<< "touch ended = " << ruis::Vec2r(p.x * scale, p.y * scale).rounded() << std::endl)
		handle_mouse_button(
				ruisapp::inst(),
				false,
				ruis::Vec2r(p.x * scale, p.y * scale).rounded(),
				ruis::MouseButton_e::LEFT,
				0 // TODO: id
			);
	}
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event{
	// TODO:
}

@end

void application::set_fullscreen(bool enable){
	auto& ww = get_impl(this->window_pimpl);
	UIWindow* w = ww.window;

	float scale = [[UIScreen mainScreen] scale];

	using std::round;

	if(enable){
		if( [[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0f ) {
			CGRect rect = w.frame;
			w.rootViewController.view.frame = rect;
		}
		update_window_rect(
				ruis::rectangle(
						ruis::vector2(0),
						ruis::vector2(
								round(w.frame.size.width * scale),
								round(w.frame.size.height * scale)
							)
					)
			);
		w.windowLevel = UIWindowLevelStatusBar;
	}else{
		CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;

		if( [[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0f ) {
			CGRect rect = w.frame;
			rect.origin.y += statusBarSize.height;
			rect.size.height -= statusBarSize.height;
			w.rootViewController.view.frame = rect;
		}

		update_window_rect(
				ruis::rectangle(
						ruis::vector2(0),
						ruis::vector2(
								round(w.frame.size.width * scale),
								round((w.frame.size.height - statusBarSize.height) * scale)
							)
					)
			);
		w.windowLevel = UIWindowLevelNormal;
	}
}

void application::quit()noexcept{
	//TODO:
}

namespace{
ruis::real getDotsPerInch(){
	float scale = [[UIScreen mainScreen] scale];

	ruis::real value;

	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
		value = 132 * scale;
	} else if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
		value = 163 * scale;
	} else {
		value = 160 * scale;
	}
	LOG([&](auto&o){o << "dpi = " << value << std::endl;})
	return value;
}
}

namespace{
ruis::real getDotsPerDp(){
	float scale = [[UIScreen mainScreen] scale];

	//TODO: use get_pixels_per_pp() function from ruis util

	return ruis::real(scale);
}
}

application::application(std::string name, const window_params& wp) :
		name(name),
		window_pimpl(utki::makeUnique<WindowWrapper>(wp)),
		gui(utki::make_shared<ruis::context>(
				utki::make_shared<ruis::render_opengles::renderer>(),
				utki::make_shared<ruis::updater>(),
				[this](std::function<void()> a){
					auto p = reinterpret_cast<NSInteger>(new std::function<void()>(std::move(a)));

					dispatch_async(dispatch_get_main_queue(), ^{
						std::unique_ptr<std::function<void()>> m(reinterpret_cast<std::function<void()>*>(p));
						(*m)();
					});
				},
				[this](ruis::mouse_cursor){},
				getDotsPerInch(),
				getDotsPerDp()
			)),
		storage_dir("") //TODO: initialize to proper value
{
	this->set_fullscreen(false);//this will intialize the viewport
}

void application::swap_frame_buffers(){
	//do nothing
}

void application::show_virtual_keyboard()noexcept{
	//TODO:
}

void application::hide_virtual_keyboard()noexcept{
	//TODO:
}

std::unique_ptr<papki::file> application::get_res_file(const std::string& path)const{
	std::string dir([[[NSBundle mainBundle] resourcePath] fileSystemRepresentation]);

//	TRACE(<< "res path = " << dir << std::endl)

	auto rdf = std::make_unique<papki::root_dir_file>(std::make_unique<papki::fs_file>(), dir + "/");
	rdf->setPath(path);

	return std::move(rdf);
}

void application::set_mouse_cursor_visible(bool visible){
	//do nothing
}
