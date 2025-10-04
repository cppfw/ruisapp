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

#include "globals.hxx"

ANativeActivity* globals_wrapper::native_activity = nullptr;

void globals_wrapper::create(ANativeActivity* activity)
{
	utki::assert(activity, SL);
	utki::assert(!activity->instance, SL);

	utki::assert(!globals_wrapper::native_activity, SL);

	try {
		globals_wrapper::native_activity = activity;

		activity->instance = new globals_wrapper();

		auto& glob = get_glob();

		glob.app = ruisapp::application_factory::make_application(
			0, // argc
			nullptr // argv
		);

		// On android it makes no sense if the app doesn't show any GUI, since it is not possible to
		// launch an android app as command line app.
		utki::assert(glob.app, SL);

		// Set the main loop event flag to call the update() for the first time if there
		// were any updateables started during creating application
		// object.
		glob.main_loop_event_fd.set();
	} catch (...) {
		globals_wrapper::native_activity = nullptr;
		throw;
	}
}

void globals_wrapper::destroy()
{
	utki::assert(globals_wrapper::native_activity, SL);
	utki::assert(globals_wrapper::native_activity->instance, SL);

	auto wrapper = static_cast<globals_wrapper*>(globals_wrapper::native_activity->instance);
	delete wrapper;

	globals_wrapper::native_activity->instance = nullptr;
	globals_wrapper::native_activity = nullptr;
}

namespace {
int on_queue_has_messages(
	int fd, //
	int events,
	void* data
)
{
	auto& glob = get_glob();

	while (auto m = glob.ui_queue.pop_front()) {
		m();
	}

	return 1; // 1 means do not remove descriptor from looper
}
} // namespace

namespace {
int on_update_timer_expired(
	int fd, //
	int events,
	void* data
)
{
	//	utki::log_debug([&](auto&o){o << "on_update_timer_expired(): invoked" <<
	// std::endl;});

	auto& glob = get_glob();

	utki::assert(glob.app, SL);
	auto& glue = get_glue(*glob.app);

	uint32_t dt = glue.updater.get().update();
	if (dt == 0) {
		// do not arm the timer and do not clear the flag
	} else {
		glob.main_loop_event_fd.clear();
		glob.timer.arm(dt);
	}

	// after updating need to re-render everything
	glue.render();

	//	utki::log_debug([&](auto&o){o << "on_update_timer_expired(): armed timer for " << dt
	//<< std::endl;});

	return 1; // 1 means do not remove descriptor from looper
}
} // namespace

globals_wrapper::globals_wrapper()
{
	// add timer descriptor to looper, this is needed for updatable to work
	if (ALooper_addFd(
			this->looper,
			this->main_loop_event_fd.get_fd(),
			ALOOPER_POLL_CALLBACK,
			ALOOPER_EVENT_INPUT,
			&on_update_timer_expired,
			0
		) == -1)
	{
		throw std::runtime_error("failed to add timer descriptor to looper");
	}

	// add UI message queue descriptor to looper
	if (ALooper_addFd(
			this->looper,
			this->ui_queue.get_handle(),
			ALOOPER_POLL_CALLBACK,
			ALOOPER_EVENT_INPUT,
			&on_queue_has_messages,
			0
		) == -1)
	{
		throw std::runtime_error("failed to add UI message queue descriptor to looper");
	}
}

globals_wrapper::~globals_wrapper()
{
	// remove UI message queue descriptor from looper
	ALooper_removeFd(
		this->looper, //
		this->ui_queue.get_handle()
	);

	// remove main_loop_event_fd from looper
	ALooper_removeFd(
		this->looper, //
		this->main_loop_event_fd.get_fd()
	);
}
