#include "android_globals.hxx"

void android_globals_wrapper::create(ANativeActivity* native_activity)
{
	utki::assert(native_activity, SL);
	utki::assert(!native_activity->instance, SL);

	utki::assert(!android_globals_wrapper::native_activity, SL);

	try {
		android_globals_wrapper::native_activity = native_activity;

		native_activity->instance = new android_globals_wrapper();
	} catch (...) {
		android_globals_wrapper::native_activity = nullptr;
		throw;
	}
}

void android_globals_wrapper::destroy()
{
	utki::assert(android_globals_wrapper::native_activity, SL);
	utki::assert(android_globals_wrapper::native_activity->instance, SL);

	auto wrapper = static_cast<android_globals_wrapper*>(android_globals_wrapper::native_activity->instance);
	delete wrapper;

	android_globals_wrapper::native_activity->instance = nullptr;
	android_globals_wrapper::native_activity = nullptr;
}
