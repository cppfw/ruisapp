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
