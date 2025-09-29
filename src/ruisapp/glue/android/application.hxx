#pragma once

#include <utki/destructable.hpp>

#include "../../application.hpp"
#include "../../window.hpp"

namespace {
class app_window : public ruisapp::window
{
public:
	utki::shared_ref<native_window> ruis_native_Window;

};
} // namespace

namespace {
class application_glue : public utki::destructable
{
	const utki::version_duplet gl_version;

	// Only one window on android.
	std::optional<app_window> window;
public:
	utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

	application_glue(utki::version_duplet gl_version);
};
} // namespace

namespace {
inline application_glue& get_glue(ruisapp::application& app)
{
	return static_cast<application_glue&>(app.pimpl.get());
}
} // namespace

namespace {
inline application_glue& get_glue()
{
	return get_glue(ruisapp::application::inst());
}
} // namespace
