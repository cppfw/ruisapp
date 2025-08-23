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

#include <memory>

#include <papki/file.hpp>
#include <r4/vector.hpp>
#include <ruis/config.hpp>
#include <ruis/gui.hpp>
#include <ruis/util/key.hpp>
#include <utki/config.hpp>
#include <utki/destructable.hpp>
#include <utki/flags.hpp>
#include <utki/singleton.hpp>
#include <utki/unique_ref.hpp>
#include <utki/util.hpp>

#include "config.hpp"
#include "window.hpp"

namespace ruisapp {

/**
 * @brief Base singleton class of application.
 * An application should subclass this class and return an instance from the
 * application_factory, see application_factory for details.
 * When instance of this class is created it also creates a window and
 * initializes rendering API (e.g. OpenGL or OpenGL ES).
 */
class application : public utki::intrusive_singleton<application>
{
	friend singleton_type;
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
	static instance_type instance;

public:
	const utki::unique_ref<utki::destructable> pimpl;

private:
	std::vector<utki::unique_ref<window>> windows_v;

public:
	const std::string name;

private:
	// TODO: remove
	std::unique_ptr<utki::destructable> window_pimpl;
	friend const decltype(window_pimpl)& get_window_pimpl(application& app);

private:
	// TODO: remove
	void swap_frame_buffers();

public:
	window& get_main_window()
	{
		utki::assert(!this->windows().empty(), SL);
		return this->windows().front();
	}

	/**
	 * @brief Get application windows.
	 * It is guaranteed that at least one window always exists,
	 * so the returned span's size is guaranteed to be >= 1.
	 * @return A span of application windows. Size of the span is always >= 1.
	 */
	utki::span<const utki::unique_ref<window>> windows()
	{
		return this->windows_v;
	}

	// TODO: move to window
	ruis::gui gui;

public:
	/**
	 * @brief Create file interface into resources storage.
	 * This function creates a ruis's standard file interface to read
	 * application's resources.
	 * @param path - file path to initialize the file interface with.
	 * @return Instance of the file interface into the resources storage.
	 */
	std::unique_ptr<papki::file> get_res_file(std::string_view path = {}) const;

public:
	/**
	 * @brief Aggregation of application directory locations.
	 * See https://www.freedesktop.org/software/systemd/man/file-hierarchy.html#Home%20Directory
	 */
	struct directories {
		std::string cache;
		std::string config;
		std::string state;
	};

	/**
	 * @brief Application directory locations.
	 * The directories are typical paths for the application use.
	 *
	 * See https://www.freedesktop.org/software/systemd/man/file-hierarchy.html#Home%20Directory for more info.
	 *
	 * The directories are not necessarily existing, before creating any files in those
	 * locations, make sure to create them, e.g. with std::filesystem::create_directories().
	 */
	const directories directory;

private:
	// TODO: make it window rectangle and track viewport separately,
	//       use top-left coordinate system

	// this is a viewport rectangle in coordinates that are as follows: x grows
	// right, y grows up.

	// TODO: move to window
	ruis::rect cur_window_rect = {0, 0, 0, 0};

public:
	// TODO: mve to window
	const ruis::vector2& window_dims() const noexcept
	{
		return this->cur_window_rect.d;
	}

private:
	void render();

	friend void render(application& app);

	// TODO: move to window
	void update_window_rect(const ruis::rect& rect);

	friend void update_window_rect(
		application& app, //
		const ruis::rect& rect
	);

	// pos is in usual window coordinates, y goes down.
	void handle_mouse_move(
		const r4::vector2<float>& pos, //
		unsigned id
	)
	{
		this->gui.send_mouse_move(
			pos, //
			id
		);
	}

	friend void handle_mouse_move(
		application& app, //
		const r4::vector2<float>& pos,
		unsigned id
	);

	// pos is in usual window coordinates, y goes down.
	void handle_mouse_button(
		bool is_down, //
		const r4::vector2<float>& pos,
		ruis::mouse_button button,
		unsigned id
	)
	{
		this->gui.send_mouse_button(
			is_down, //
			pos,
			button,
			id
		);
	}

	friend void handle_mouse_button(
		application& app, //
		bool is_down,
		const r4::vector2<float>& pos,
		ruis::mouse_button button,
		unsigned id
	);

	void handle_mouse_hover(
		bool is_hovered, //
		unsigned id
	)
	{
		this->gui.send_mouse_hover(
			is_hovered, //
			id
		);
	}

	friend void handle_mouse_hover(
		application& app, //
		bool is_hovered,
		unsigned pointer_id
	);

public:
	/**
	 * @brief Application parameters.
	 */
	struct parameters {
		/**
		 * @brief Application's name.
		 * This name is not to be presented to the user, instead it is the name for
		 * itentifying the application within the system.
		 * For example, this name will be used to name application's config directories etc.
		 * So, the name should be picked so that it does not collide with others.
		 */
		std::string name;

		/**
		 * @brief Graphics API version.
		 * The version of the graphics API to initialize.
		 * All windows will share the same graphics API.
		 * A shared graphics API context will be created for each window.
		 * Version value of 0.0 means to initialize minimal supported version of the graphics API.
		 * Minimal supported versions of various graphics APIs:
		 * - OpenGL 2.0
		 * - OpenGL ES 2.0
		 * - TODO: add more APIs
		 */
		// clang-format off
		utki::version_duplet graphics_api_version = {
			.major = 0,
			.minor = 0
		};
		// clang-format on

		/**
		 * @brief Application windows to create.
		 * At least one window should be specified.
		 */
		std::vector<window_parameters> windows;
	};

protected:
	/**
	 * @brief Application constructor.
	 * @param params - application parameters.
	 * @throw std::invalid_argument - in case list of windows to create is empty.
	 */
	application(parameters params);

public:
	application(const application&) = delete;
	application& operator=(const application&) = delete;

	application(application&&) = delete;
	application& operator=(application&&) = delete;

	~application() override = default;

	/**
	 * @brief Bring up the virtual keyboard.
	 * On mobile platforms this function will summon the on-screen keyboard.
	 * On desktop platforms this function does nothing.
	 */
	// TODO: move to window?
	void show_virtual_keyboard() noexcept;

	/**
	 * @brief Hide virtual keyboard.
	 * On mobile platforms this function hides the on-screen keyboard.
	 * On desktop platforms this function does nothing.
	 */
	// TODO: move to window?
	void hide_virtual_keyboard() noexcept;

private:
	// The idea with unicode_resolver parameter is that we don't want to calculate
	// the unicode unless it is really needed, thus postpone it as much as
	// possible.
	void handle_character_input(
		const ruis::gui::input_string_provider& string_provider, //
		ruis::key key_code
	)
	{
		this->gui.send_character_input(
			string_provider, //
			key_code
		);
	}

	friend void handle_character_input(
		application& app, //
		const ruis::gui::input_string_provider& string_provider,
		ruis::key key_code
	);

	void handle_key_event(
		bool is_down, //
		ruis::key key_code
	);

	friend void handle_key_event(
		application& app, //
		bool is_down,
		ruis::key key_code
	);

public:
	/**
	 * @brief Requests application to exit.
	 * This function posts an exit message to the applications message queue.
	 * The message will normally be handled on the next UI cycle and as a result
	 * the main loop will be terminated and application will exit. The Application
	 * object will be destroyed and all resources freed.
	 */
	void quit() noexcept;

private:
	// TODO: not used in linux, move to window implementation
	r4::rectangle<int> before_fullscreen_window_rect{0, 0, 0, 0};

public:
	/**
	 * @brief Check if application currently runs in fullscreen mode.
	 * @return true if application is in fullscreen mode.
	 * @return false if application is in windowed mode.
	 */
	// TODO: remove
	bool is_fullscreen() const noexcept;

	/**
	 * @brief Set/unset fullscreen mode.
	 * @param enable - whether to enable or to disable fullscreen mode.
	 */
	// TODO: remove
	void set_fullscreen(bool enable);

	/**
	 * @brief Show/hide mouse cursor.
	 * @param visible - whether to show (true) or hide (false) mouse cursor.
	 */
	// TODO: remove
	void set_mouse_cursor_visible(bool visible);

	/**
	 * @brief Get dots per density pixel (dp) for given display parameters.
	 * The size of the dp for desktop displays should normally be equal to one
	 * pixel. For hand held devices size of the dp depends on physical screen size
	 * and pixel resolution.
	 * @param screen_size_pixels - resolution of the display in pixels.
	 * @param screen_size_mm - size of the display in millimeters.
	 * @return Size of one display density pixel in pixels.
	 */
	static ruis::real get_pixels_per_pp(
		r4::vector2<unsigned> screen_size_pixels, //
		r4::vector2<unsigned> screen_size_mm
	);
};

inline application& inst()
{
	return application::inst();
}

/**
 * @brief Application factory registerer.
 * The object of this class registers the application factory function.
 * The application object will be constructed using the provided factory
 * function at program start.
 */
class application_factory
{
public:
	using factory_type = std::function<std::unique_ptr<application>(
		std::string_view executable, //
		utki::span<std::string_view> args
	)>;

	/**
	 * @brief Constructor.
	 * Registers the application object factory function.
	 * Only one application factory can be registered.
	 * @param factory - application factory function.
	 * @throw std::logic_error - in case a factory is already registered.
	 */
	application_factory(factory_type factory);

	/**
	 * @brief Get registered factory function.
	 * @return Registered factory function.
	 * @throw std::logic_error if no factory function was registered, i.e. application_factory instance is not created.
	 */
	static const factory_type& get_factory();

	/**
	 * @brief Create application object.
	 * @param argc - number of command line arguments.
	 * @param argv - array of command line arguments. First argument is the
	 *               executable filename.
	 */
	static std::unique_ptr<application> make_application(
		int argc, //
		const char** argv
	);

private:
	static factory_type& get_factory_internal();
};

} // namespace ruisapp
