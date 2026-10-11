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

#include <utki/destructable.hpp>

#include "../../application.hpp"
#include "../../window.hpp"

#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
	ruis::rect cur_win_rect{0, 0, 0, 0};

	using inset_value_type = ruis::internal::style_value<ruis::length>;

	// Live content insets (system bars), in pixels. Each side is a shared_ref to a
	// style_value so that the styled values returned by get_insets() reference them;
	// updating the value in place via set_insets() is observed by the application on
	// the next layout pass without re-querying. The default is undefined, matching
	// platforms with no insets (e.g. desktop); it is set on the first content-rect
	// change (on_content_rect_changed -> set_insets).
	utki::shared_ref<inset_value_type> inset_left = utki::make_shared<inset_value_type>(ruis::length{});
	utki::shared_ref<inset_value_type> inset_top = utki::make_shared<inset_value_type>(ruis::length{});
	utki::shared_ref<inset_value_type> inset_right = utki::make_shared<inset_value_type>(ruis::length{});
	utki::shared_ref<inset_value_type> inset_bottom = utki::make_shared<inset_value_type>(ruis::length{});

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

	// Update the live insets (in pixels). Called by the glue when the window
	// content rect changes (initial layout, rotation, nav bar toggle, ...).
	void set_insets(
		ruis::real left, //
		ruis::real top,
		ruis::real right,
		ruis::real bottom
	);

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
