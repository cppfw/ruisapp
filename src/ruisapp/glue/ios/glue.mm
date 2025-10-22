#include "../../application.hpp"

#include <utki/destructable.hpp>
#include <utki/util.hpp>
#include <utki/debug.hpp>

#include <papki/fs_file.hpp>
#include <papki/root_dir.hpp>

#include <sstream>
#include <stdexcept>

#include <ruis/render/opengles/context.hpp>

// include implementations
#include "application.cxx"
#include "window.cxx"

@interface AppDelegate : UIResponder <UIApplicationDelegate>{
	std::unique_ptr<ruisapp::application> app;
}

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)ui_app didFinishLaunchingWithOptions:(NSDictionary *)launch_options
{
	self->app = ruisapp::application_factory::make_application(
		0, // argc
		nullptr // argv
	);
	// On ios it makes no sense if the app doesn't show any GUI, since it is not possible to
	// launch an ios app as command line app.
	utki::assert(self->app, SL);

	return YES; // TODO: what does this YES mean?
}

- (void)applicationWillResignActive:(UIApplication *)ui_app
{
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)ui_app
{
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)ui_app
{
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)ui_app
{
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)ui_app
{
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
	utki::assert(self->app, SL);
	self->app.reset();
}

@end

int main(int argc, char * argv[]){
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	utki::scope_exit pool_scope_exit([&](){
		[pool release];
	});

	int ret = UIApplicationMain(
		argc,//
	 	argv,
		nil,
		NSStringFromClass([AppDelegate class])
	);
	
	return ret;
}

// void application::set_fullscreen(bool enable){
// 	auto& ww = get_impl(this->window_pimpl);
// 	UIWindow* w = ww.window;

// 	float scale = [[UIScreen mainScreen] scale];

// 	using std::round;

// 	if(enable){
// 		if( [[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0f ) {
// 			CGRect rect = w.frame;
// 			w.rootViewController.view.frame = rect;
// 		}
// 		update_window_rect(
// 				ruis::rect(
// 						ruis::vector2(0),
// 						ruis::vector2(
// 								round(w.frame.size.width * scale),
// 								round(w.frame.size.height * scale)
// 							)
// 					)
// 			);
// 		w.windowLevel = UIWindowLevelStatusBar;
// 	}else{
// 		CGSize statusBarSize = [[UIApplication sharedApplication] statusBarFrame].size;

// 		if( [[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0f ) {
// 			CGRect rect = w.frame;
// 			rect.origin.y += statusBarSize.height;
// 			rect.size.height -= statusBarSize.height;
// 			w.rootViewController.view.frame = rect;
// 		}

// 		update_window_rect(
// 				ruis::rect(
// 						ruis::vector2(0),
// 						ruis::vector2(
// 								round(w.frame.size.width * scale),
// 								round((w.frame.size.height - statusBarSize.height) * scale)
// 							)
// 					)
// 			);
// 		w.windowLevel = UIWindowLevelNormal;
// 	}
// }
