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

// include implementations
#include "application.cxx"
#include "window.cxx"

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

	// in order to get keyboard events we need to be foreground application
	{
		ProcessSerialNumber psn = {0, kCurrentProcess};
		OSStatus status = TransformProcessType(&psn, kProcessTransformToForegroundApplication);
		if(status != errSecSuccess){
			utki::assert(false, SL);
		}
	}

	do{
		glue.windows_to_destroy.clear();

		// main loop cycle sequence as required by ruis:
		// - update updateables
		// - render
		// - wait for events and handle them
		uint32_t millis = glue.updater.get().update();

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
						NSInteger data = [event data1];
						utki::assert(data, SL);
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

// TODO:
// namespace{
// ruis::real getDotsPerInch(){
// 	NSScreen *screen = [NSScreen mainScreen];
// 	NSDictionary *description = [screen deviceDescription];
// 	NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
// 	CGSize displayPhysicalSize = CGDisplayScreenSize(
// 			[[description objectForKey:@"NSScreenNumber"] unsignedIntValue]
// 		);

// 	ruis::real value = ruis::real(((displayPixelSize.width * 10.0f / displayPhysicalSize.width) +
// 			(displayPixelSize.height * 10.0f / displayPhysicalSize.height)) / 2.0f);
// 	value *= 2.54f;
// 	return value;
// }
// }

// namespace{
// ruis::real getDotsPerPt(){
// 	NSScreen *screen = [NSScreen mainScreen];
// 	NSDictionary *description = [screen deviceDescription];
// 	NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
// 	CGSize displayPhysicalSize = CGDisplayScreenSize(
// 			[[description objectForKey:@"NSScreenNumber"] unsignedIntValue]
// 		);

// 	r4::vector2<unsigned> resolution(displayPixelSize.width, displayPixelSize.height);
// 	r4::vector2<unsigned> screenSizeMm(displayPhysicalSize.width, displayPhysicalSize.height);

// 	return application::get_pixels_per_pp(resolution, screenSizeMm);
// }
// }

// application::application(
// 	std::string name,
// 	const window_parameters& wp
// ) :
// 	name(name),
// 	window_pimpl(std::make_unique<WindowWrapper>(wp)),
// 	gui(utki::make_shared<ruis::context>(
// 		utki::make_shared<ruis::style_provider>(
// 			utki::make_shared<ruis::resource_loader>(
// 				utki::make_shared<ruis::render::renderer>(
// 					utki::make_shared<ruis::render::opengl::context>()
// 				)
// 			)
// 		),
// 		utki::make_shared<ruis::updater>(),
// 		ruis::context::parameters{
// 			.post_to_ui_thread_function = [this](std::function<void()> a){
// 				auto& ww = get_impl(get_window_pimpl(*this));

// 				NSEvent* e = [NSEvent
// 						otherEventWithType: NSEventTypeApplicationDefined
// 						location: NSMakePoint(0, 0)
// 						modifierFlags:0
// 						timestamp:0
// 						windowNumber:0
// 						context: nil
// 						subtype: 0
// 						data1: reinterpret_cast<NSInteger>(new std::function<void()>(std::move(a)))
// 						data2: 0
// 					];

// 				[ww.applicationObjectId postEvent:e atStart:NO];
// 			},
// 			.set_mouse_cursor_function = [](ruis::mouse_cursor c){
// 				// TODO:
// 			},
// 			.units = ruis::units(
// 				getDotsPerInch(), //
// 				getDotsPerPt()
// 			)
// 		}
// 	)),
// 	directory(get_application_directories(this->name))
// {
// 	utki::log_debug([&](auto&o){o << "application::application(): enter" << std::endl;});
// 	this->update_window_rect(
// 			ruis::rect(
// 					0,
// 					0,
// 					ruis::real(wp.dims.x()),
// 					ruis::real(wp.dims.y())
// 				)
// 		);
// }
