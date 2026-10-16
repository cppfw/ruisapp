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

#pragma once

#include <algorithm>

#include <utki/destructable.hpp>

#include "../../application.hpp"
#include "../../window.hpp"

#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
	ruis::rect cur_win_rect{0, 0, 0, 0};

	using inset_value_type = ruis::internal::style_value<ruis::length>;

	// Live content insets, in pixels, returned by get_insets(): the region beyond
	// which the content is not visible (the per-side max of the system bar insets and
	// the input-method insets, combined on the Java side). Each side is a shared_ref to
	// a style_value so that the styled values returned by get_insets() reference them;
	// updating the value in place via set_insets() is observed by the application on the
	// next layout pass without re-querying.
	ruis::sides<utki::shared_ref<inset_value_type>> insets{
		utki::make_shared<inset_value_type>(ruis::length::make_px(0)), //
		utki::make_shared<inset_value_type>(ruis::length::make_px(0)), //
		utki::make_shared<inset_value_type>(ruis::length::make_px(0)), //
		utki::make_shared<inset_value_type>(ruis::length::make_px(0)) //
	};

public:
	const utki::shared_ref<native_window> ruis_native_window;

	app_window(
		utki::shared_ref<ruis::context> ruis_context, //
		utki::shared_ref<native_window> ruis_native_window
	) :
		ruisapp::window(std::move(ruis_context)),
		ruis_native_window(std::move(ruis_native_window))
	{}

	void set_win_rect(const ruis::rect& r);

	const ruis::rect& get_win_rect() const noexcept
	{
		return this->cur_win_rect;
	}

	// Override of the base window::get_insets(); returns live references to the
	// inset values owned by this window.
	ruis::sides<ruis::styled<ruis::length>> get_insets() override;

	// Set the content insets (in pixels, one per side). If the values changed, the
	// root widget is reloaded to invalidate the layout of the widget tree, so that the
	// widgets referencing the insets (e.g. a padding's borders) pick up the new values
	// on the next render.
	// Called by the glue from the Java side (via onInsetsChanged) whenever the window
	// insets change (system bars hidden/shown, e.g. on entering/leaving fullscreen, and
	// the on-screen keyboard shown/hidden - both combined on the Java side), and when
	// the window content rect changes (initial layout, rotation, ...).
	// NOTE: in edge-to-edge mode the window is not resized when the keyboard is shown
	// nor when the system bars are hidden/shown, so neither can be derived from the
	// content rect (the ANativeActivity onContentRectChanged callback); the Java-side
	// WindowInsets listener is the reliable source.
	void set_insets(ruis::sides<ruis::real> insets);

	ruis::vec2 android_win_coords_to_ruisapp_win_rect_coords(const ruis::vec2& p)
	{
		// utki::logcat_debug("p = ", p, '\n');
		// utki::logcat_debug("this->get_win_rect() = ", this->get_win_rect(), '\n');
		ruis::vec2 ret = p - this->get_win_rect().p;
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
	const utki::shared_ref<ruis::updater> updater = utki::make_shared<ruis::updater>();

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
