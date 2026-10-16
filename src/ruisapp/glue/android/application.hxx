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

	// Live content insets, in pixels, returned by get_insets(). On each side this
	// is the max of the (transparent) system-bar inset and the input-method
	// (keyboard) inset, so that the content rect inside these insets is the minimal
	// region not covered by the system bars or the keyboard. Each side is a
	// shared_ref to a style_value so that the styled values returned by get_insets()
	// reference them; updating the value in place via update_insets() is observed by
	// the application on the next layout pass without re-querying.
	utki::shared_ref<inset_value_type> inset_left = utki::make_shared<inset_value_type>(ruis::length::make_px(0));
	utki::shared_ref<inset_value_type> inset_top = utki::make_shared<inset_value_type>(ruis::length::make_px(0));
	utki::shared_ref<inset_value_type> inset_right = utki::make_shared<inset_value_type>(ruis::length::make_px(0));
	utki::shared_ref<inset_value_type> inset_bottom = utki::make_shared<inset_value_type>(ruis::length::make_px(0));

	// Raw component insets (in pixels) used to compute the live insets above:
	//   inset_side = max(system_bar_inset_side, keyboard_inset_side)
	// system_bar_inset_* is set from on_content_rect_changed (the transparent
	// system bars, e.g. status / navigation bar; zero when the window does not
	// extend behind them, and on desktop). keyboard_inset_* is set from the
	// Java-side WindowInsets listener (the on-screen keyboard; zero when it is
	// hidden). NOTE: in edge-to-edge mode the window is not resized when the
	// keyboard is shown, so the keyboard insets cannot be derived from the content
	// rect (the ANativeActivity onContentRectChanged callback); the WindowInsets
	// listener is the reliable source.
	ruis::real system_bar_inset_left = 0;
	ruis::real system_bar_inset_top = 0;
	ruis::real system_bar_inset_right = 0;
	ruis::real system_bar_inset_bottom = 0;
	ruis::real keyboard_inset_left = 0;
	ruis::real keyboard_inset_top = 0;
	ruis::real keyboard_inset_right = 0;
	ruis::real keyboard_inset_bottom = 0;

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
	// inset values (the max of the system-bar and keyboard insets) owned by this
	// window.
	ruis::sides<ruis::styled<ruis::length>> get_insets() override;

	// Set the system-bar insets component (in pixels) and refresh the live
	// insets. Called by the glue when the window content rect changes (initial
	// layout, rotation, nav bar toggle, ...).
	void set_insets(
		ruis::real left, //
		ruis::real top,
		ruis::real right,
		ruis::real bottom
	);

	// Set the keyboard (input method) insets component (in pixels) and refresh the
	// live insets. Called by the glue from the Java side (via
	// onKeyboardInsetsChanged) when the on-screen keyboard is shown or hidden.
	void set_keyboard_insets(
		ruis::real left, //
		ruis::real top,
		ruis::real right,
		ruis::real bottom
	);

	// Recompute the live insets as the max of the system-bar and keyboard
	// components.
	void update_insets();

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
