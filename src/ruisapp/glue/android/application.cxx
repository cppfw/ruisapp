#include "application.hxx"

#include "android_globals.hxx"
#include "asset_file.hxx"

application_glue::application_glue(utki::version_duplet gl_version) :
	gl_version(std::move(gl_version))
{}

std::unique_ptr<papki::file> ruisapp::application::get_res_file(std::string_view path) const
{
	utki::assert(globals_wrapper::native_activity, SL);
	utki::assert(globals_wrapper::native_activity->assetManager, SL);

	return std::make_unique<asset_file>(
		globals_wrapper::native_activity->assetManager, //
		path
	);
}
