#include "application.hxx"

#include <Shlobj.h> // needed for SHGetFolderPathA()

namespace {
ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	// the variable is initialized via output argument, so no need
	// to initialize it here
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	std::array<CHAR, MAX_PATH> path;
	if (SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, path.data()) != S_OK) {
		throw std::runtime_error("failed to get user's profile directory.");
	}

	path.back() = '\0'; // null-terminate the string just in case

	std::string home_dir(path.data(), strlen(path.data()));
	ASSERT(!home_dir.empty())

	std::replace(
		home_dir.begin(), //
		home_dir.end(),
		'\\',
		'/'
	);

	home_dir = papki::as_dir(home_dir);

	home_dir.append(1, '.').append(app_name).append(1, '/');

	ruisapp::application::directories dirs;

	dirs.cache = utki::cat(home_dir, "cache/");
	dirs.config = utki::cat(home_dir, "config/");
	dirs.state = utki::cat(home_dir, "state/");

	return dirs;
}
} // namespace

ruisapp::application::application(parameters params) :
	application(
		utki::make_unique<application_glue>(params.graphics_api_version), //
		get_application_directories(params.name),
		std::move(params)
	)
{}
