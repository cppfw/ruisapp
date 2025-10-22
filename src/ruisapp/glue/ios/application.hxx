#pragma once

#include <utki/destructable.hpp>

#include "../../application.hpp"

#include "window.hxx"

namespace{
class app_window : public ruisapp::window{
public:
    const utki::shared_ref<native_window> ruis_native_window;

    app_window(
        utki::shared_ref<ruis::context> ruis_context, //
        utki::shared_ref<native_window> ruis_native_window
    ) :
		ruisapp::window(std::move(ruis_context)),
		ruis_native_window(std::move(ruis_native_window))
	{
        this->ruis_native_window.get().set_app_window(this);
    }
};
}

namespace{
class application_glue : public utki::destructable{
    const utki::version_duplet gl_version;

    // Only one window on ios.
	std::optional<app_window> window;

public:
    const utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

    application_glue(utki::version_duplet gl_version);

    app_window& make_window(ruisapp::window_parameters window_params);

    void render();

    app_window* get_window()
	{
		if (this->window.has_value()) {
			return &this->window.value();
		}
		return nullptr;
	}
};
}

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
