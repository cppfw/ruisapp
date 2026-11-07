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

#include "application.hpp"

#include <exception>

#include <fsif/native_file.hpp>
#include <fsif/root_dir.hpp>
#include <ruis/widget/widget.hpp>
#include <tml/crawler.hpp>
#include <utki/config.hpp>
#include <utki/debug.hpp>

using namespace ruisapp;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
application::instance_type application::instance;

bool application::is_constructed_v = false;

application_factory::factory_type& application_factory::get_factory_internal()
{
	static application_factory::factory_type f;
	return f;
}

const application_factory::factory_type& application_factory::get_factory()
{
	auto& f = get_factory_internal();
	if (!f) {
		throw std::logic_error("no application factory registered");
	}
	return f;
}

std::unique_ptr<application> application_factory::make_application(
	int argc, //
	const char** argv
)
{
	auto cli_args = utki::make_span(argv, argc);

	if (cli_args.empty()) {
		return get_factory()(std::string_view(), {});
	}

	std::string_view executable = cli_args.front();

	std::vector<std::string_view> args;
	for (const auto& a : cli_args.subspan(1)) {
		args.emplace_back(a);
	}

	return get_factory()(executable, args);
}

application_factory::application_factory(factory_type factory)
{
	auto& f = this->get_factory_internal();
	if (f) {
		throw std::logic_error("application factory is already registered");
	}
	f = std::move(factory);
}

application::application(private_parameters params) :
	pimpl(std::move(params.pimpl)),
	name(std::move(params.params.name)),
	directory(std::move(params.directories)),
	mount_ruis_res_pack(params.params.mount_ruis_res_pack)
{
	if (auto& sp = this->get_shared_style_provider(); sp) {
		// Platform supports shared style_provider.

		if (this->mount_ruis_res_pack) {
			sp->res_loader.get().mount_ruis_res_pack(this->get_res_file());
		}
	}

	is_constructed_v = true;
}

application::~application()
{
	is_constructed_v = false;
}

#if CFG_OS_NAME != CFG_OS_NAME_ANDROID && CFG_OS_NAME != CFG_OS_NAME_IOS
utki::unique_ref<fsif::file> application::get_res_file(std::string_view path) const
{
	return utki::make_unique<fsif::native_file>(path);
}
#endif

ruisapp::window& application::make_window(window_parameters window_params)
{
	if (window_params.style_provider) {
		const auto& default_sp = this->get_default_style_provider();

		if (!default_sp) {
			throw std::invalid_argument(
				"supplied a custom style_provider on a platform which does not support it (i.e. platform allowing creating only one application window)"
			);
		}

		if (&window_params.style_provider->res_loader.get() != &default_sp->res_loader.get()) {
			throw std::invalid_argument("supplied custom_provider has inconsistent resource_loader instance");
		}
	}

	auto& win = this->make_window_internal(std::move(window_params));

	// By choice, the VSYNC is enabled by default.
	// This call will synchornize the is_vsync_enabled_v field of the native window
	// with the actual VSYNC state of the newly created window.
	win.gui.context.get().ren().ctx().set_vsync_enabled(true);

	if (!this->get_shared_style_provider()) {
		// The platform does not support shared style_provider.

		// The platform should support only one window which has just been created.
		utki::assert(this->get_windows().size() == size_t(1));

		auto& sp = win.gui.context.get().style();

		if (this->mount_ruis_res_pack) {
			// The platform doesn't have shared style provider, so we need to
			// mount ruis res pack and set the theme to the newly created window.
			win.gui.ctx().loader().mount_ruis_res_pack(this->get_res_file());
		}

		if (this->default_theme.has_value()) {
			sp.set(this->default_theme.value());
		}
	}

	return win;
}

const std::shared_ptr<ruis::style_provider>& application::get_default_style_provider()
{
	const auto& shared = this->get_shared_style_provider();
	if (shared) {
		return shared;
	}

	auto windows = this->get_windows();
	if (!windows.empty()) {
		return windows.front().get().gui.context.get().style_provider.to_shared_ptr();
	}

	// return nullptr because shared style provider is nullptr
	return shared;
}

void application::set_theme(ruis::theme th)
{
	this->default_theme = th;

	auto& sp = this->get_default_style_provider();

	if (!sp) {
		return;
	}

	// Set theme to the style_provider.
	sp->set(th);

	// Reload window GUIs.
	for (auto& w : this->get_windows()) {
		w.get().gui.get_root().reload();
	}
}

namespace {
// success response: ok
tml::forest make_app_ok()
{
	return {tml::tree(tml::leaf("ok"))};
}

// error response: error { <diagnostic> }
tml::forest make_app_error(std::string diagnostic)
{
	return {tml::tree(tml::leaf("error"), tml::forest{tml::tree(tml::leaf(std::move(diagnostic)))})};
}
} // namespace

tml::forest application::probe(const tml::tree& command)
{
	// The parameters of the 'for_window' command are read with the tml::crawler,
	// so a missing or malformed parameter is detected by the exception thrown by the
	// crawler (or by the tml::leaf numeric conversion), which is caught and turned
	// into an error response carrying the exception message as the diagnostic.
	if (command.value == "get_windows") {
		const auto windows = this->get_windows();
		return {tml::tree(tml::leaf("windows_count"), tml::forest{tml::tree(tml::leaf(unsigned(windows.size())))})};
	} else if (command.value == "for_window") {
		const auto windows = this->get_windows();
		// the order of the children is strict: first 'index', then the command to forward,
		// so the command is the child immediately after the 'index' node
		try {
			const tml::const_crawler crw(command.children);
			const unsigned index = unsigned(crw.to("index").in().get().value.to_uint32());
			// crw is still at the index node, because .in() creates a new crawler instance
			const tml::tree& sub_command = crw.next().get();
			if (index >= windows.size()) {
				return make_app_error("window_not_found");
			}
			return windows[index].get().gui.probe(sub_command);
		} catch (const std::exception& e) {
			return make_app_error(e.what());
		}
	} else if (command.value == "quit") {
		this->quit();
		return make_app_ok();
	}

	return make_app_error("unknown_command");
}
