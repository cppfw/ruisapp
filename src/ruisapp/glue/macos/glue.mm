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
