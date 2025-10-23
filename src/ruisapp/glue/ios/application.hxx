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

#include "window.hxx"

namespace {
class app_window : public ruisapp::window
{
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
} // namespace

namespace {
class application_glue : public utki::destructable
{
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
