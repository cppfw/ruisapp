#include "application.hxx"

#include "android_globals.hxx"
#include "asset_file.hxx"

application_glue::application_glue(utki::version_duplet gl_version) :
	gl_version(std::move(gl_version))
{}

app_window& application_glue::make_window(ruisapp::window_parameters window_params){
	utki::assert(!this->window.has_value(), SL);

	// TODO:

	utki::assert(this->window.has_value(), SL);
	return this->window.value();
}

void application_glue::render(){
	if(this->window.has_value()){
		this->window.value().render();
	}
}

std::unique_ptr<papki::file> ruisapp::application::get_res_file(std::string_view path) const
{
	utki::assert(globals_wrapper::native_activity, SL);
	utki::assert(globals_wrapper::native_activity->assetManager, SL);

	return std::make_unique<asset_file>(
		globals_wrapper::native_activity->assetManager, //
		path
	);
}

ruisapp::window& ruisapp::application::make_window(ruisapp::window_parameters window_params){
	auto& glue = get_glue(*this);
	return glue.make_window(std::move(window_params));
}
