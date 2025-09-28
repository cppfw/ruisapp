#include "application.hxx"

#include "android_globals.hxx"
#include "asset_file.hxx"

std::unique_ptr<papki::file> ruisapp::application::get_res_file(std::string_view path) const
{
	utki::assert(android_globals_wrapper::native_activity, SL);
	utki::assert(android_globals_wrapper::native_activity->assetManager, SL);

	return std::make_unique<asset_file>(
		android_globals_wrapper::native_activity->assetManager, //
		path
	);
}
