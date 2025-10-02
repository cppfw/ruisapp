#pragma once

#include <utki/destructable.hpp>

#include "../../application.hpp"
#include "../../window.hpp"

#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
	ruis::rect cur_win_rect{0, 0, 0, 0};

public:
	utki::shared_ref<native_window> ruis_native_window;

	app_window(utki::shared_ref<ruis::context> ruis_context, utki::shared_ref<native_window> ruis_native_window) :
		ruisapp::window(std::move(ruis_context)),
		ruis_native_window(std::move(ruis_native_window))
	{}

	void set_win_rect(const ruis::rect& r);

	const ruis::rect& get_win_rect() const noexcept
	{
		return this->cur_win_rect;
	}

	ruis::vector2 android_win_coords_to_ruisapp_win_rect_coords(const ruis::vector2& p)
	{
		// utki::logcat_debug("p = ", p, '\n');
		// utki::logcat_debug("this->get_win_rect() = ", this->get_win_rect(), '\n');
		ruis::vector2 ret = p - this->get_win_rect().p;
		using std::round;
		return round(ret);
	}
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

	app_window& make_window(ruisapp::window_parameters window_params);

	void render();

	void create_window_surface(ANativeWindow& android_window)
	{
		if (this->window.has_value()) {
			this->window.value().ruis_native_window.get().create_surface(android_window);
		}
	}

	void destroy_window_surface()
	{
		if (this->window.has_value()) {
			this->window.value().ruis_native_window.get().destroy_surface();
		}
	}

	app_window* get_window()
	{
		if (this->window.has_value()) {
			return &this->window.value();
		}
		return nullptr;
	}
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
