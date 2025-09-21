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

#include <ratio>

#include <papki/fs_file.hpp>
#include <ruis/context.hpp>
#include <ruis/util/util.hpp>
#include <utki/destructable.hpp>
#include <utki/windows.hpp>

#ifdef RUISAPP_RENDER_OPENGL
#	include <ruis/render/opengl/context.hpp>
#elif defined(RUISAPP_RENDER_OPENGLES)
#	include <EGL/egl.h>
#	include <ruis/render/opengles/context.hpp>
#else
#	error "Unknown graphics API"
#endif

#include "../../application.hpp"

#include "application.hxx"

// include implementations
#include "display.cxx"
#include "application.cxx"
#include "window.cxx"

using namespace ruisapp;

namespace {
void winmain(
	int argc, //
	const char** argv
)
{
	auto app = ruisapp::application_factory::make_application(argc, argv);
	if (!app) {
		// Not an error. The application just did not show any GUI to the user and exited normally.
		return;
	}

	auto& glue = get_glue(*app);

	while (!glue.quit_flag.load()) {
		glue.windows_to_destroy.clear();

		// main loop cycle sequence as required by ruis:
		// - update updateables
		// - render
		// - wait for events and handle them/next cycle
		uint32_t timeout = glue.updater.get().update();

		glue.render();

		DWORD status = MsgWaitForMultipleObjectsEx(
			0,// number of handles to wait for
			nullptr,// we do not wait for any handles
			timeout,
			QS_ALLINPUT, // we wait for ALL inpuit events
			MWMO_INPUTAVAILABLE // we wait for inpuit events
		);

		if (status == WAIT_OBJECT_0) {
			// not a timeout, some events happened

			MSG msg;
			while (PeekMessage(
				&msg,//
				NULL, // retrieve messages for any window, as well as thread messages
				0, // no message filtering, retrieve all messages
				0, // no message filtering, retrieve all messages
				PM_REMOVE // remove messages from queue after processing by PeekMessage()
			)) {
				if (msg.message == WM_QUIT) {
					glue.quit_flag.store(true);
					break;
				}else if (msg.message == WM_USER)
				{
					std::unique_ptr<std::function<void()>> m(
						// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
						reinterpret_cast<std::function<void()>*>(msg.lParam)
					);
					(*m)();
					continue;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
}
} // namespace

int WINAPI WinMain(
	HINSTANCE h_instance, // Instance
	HINSTANCE h_prev_instance, // Previous Instance
	LPSTR lp_cmd_line, // Command Line Parameters
	int n_cmd_show // Window Show State
)
{
	winmain(
		__argc, //
		const_cast<const char**>(__argv)
	);

	return 0;
}

int main(int argc, const char** argv)
{
	winmain(argc, argv);

	return 0;
}
