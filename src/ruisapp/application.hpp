/*
mordavokne - morda GUI adaptation layer

Copyright (C) 2016-2021  Ivan Gagis <igagis@gmail.com>

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

#include <ruis/gui.hpp>
#include <ruis/util/key.hpp>
#include <papki/file.hpp>
#include <r4/vector.hpp>
#include <utki/config.hpp>
#include <utki/destructable.hpp>
#include <utki/flags.hpp>
#include <utki/singleton.hpp>

#include "config.hpp"

namespace ruisapp {

/**
 * @brief Desired window parameters.
 */
struct window_params {
	/**
	 * @brief Desired dimensions of the window
	 */
	r4::vector2<unsigned> dims;

	// TODO: add window title string

	enum class buffer_type {
		depth,
		stencil,

		enum_size
	};

	/**
	 * @brief Flags describing desired buffers for OpneGL context.
	 */
	utki::flags<buffer_type> buffers = false;

	enum class graphics_api {
		gl_2_0,
		gl_2_1,
		gl_3_0,
		gl_3_1,
		gl_3_2,
		gl_3_3,
		gl_4_0,
		gl_4_1,
		gl_4_2,
		gl_4_3,
		gl_4_4,
		gl_4_5,
		gl_4_6,
		gles_2_0,
		gles_3_0,

		enum_size
	};

	graphics_api graphics_api_request =
#if M_OS_NAME == M_OS_NAME_ANDROID || M_OS_NAME == M_OS_NAME_IOS
		graphics_api::gles_2_0
#elif M_OS == M_OS_WINDOWS || M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
		graphics_api::gl_2_0
#else
#	error "unknown OS"
#endif
		;

	window_params(r4::vector2<unsigned> dims) :
		dims(dims)
	{}
};

/**
 * @brief Base singleton class of application.
 * An application should subclass this class and return an instance from the
 * application factory function create_application(), see application.hpp for
 * details. When instance of this class is created it also creates a window and
 * initializes rendering API (e.g. OpenGL or OpenGL ES).
 */
class application : public utki::intrusive_singleton<application>
{
	friend singleton_type;
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
	static instance_type instance;

public:
	const std::string name;

private:
	std::unique_ptr<utki::destructable> window_pimpl;

	friend const decltype(window_pimpl)& get_window_pimpl(application& app);

private:
	void swap_frame_buffers();

public:
	ruis::gui gui;

public:
	/**
	 * @brief Create file interface into resources storage.
	 * This function creates a morda's standard file interface to read
	 * application's resources.
	 * @param path - file path to initialize the file interface with.
	 * @return Instance of the file interface into the resources storage.
	 */
	std::unique_ptr<papki::file> get_res_file(const std::string& path = std::string()) const;

public:
	/**
	 * @brief Storage directory path.
	 * Path to the application's storage directory. This is the directory
	 * where application generated files are to be stored, like configurations,
	 * saved states, etc.
	 * The path is always ended with '/' character.
	 */
	const std::string storage_dir;

private:
	// this is a viewport rectangle in coordinates that are as follows: x grows
	// right, y grows up.
	ruis::rectangle curWinRect = ruis::rectangle(0, 0, 0, 0);

public:
	const ruis::vector2& window_dims() const noexcept
	{
		return this->curWinRect.d;
	}

private:
	void render();

	friend void render(application& app);

	void update_window_rect(const ruis::rectangle& rect);

	friend void update_window_rect(application& app, const ruis::rectangle& rect);

	// pos is in usual window coordinates, y goes down.
	void handle_mouse_move(const r4::vector2<float>& pos, unsigned id)
	{
		this->gui.send_mouse_move(pos, id);
	}

	friend void handle_mouse_move(application& app, const r4::vector2<float>& pos, unsigned id);

	// pos is in usual window coordinates, y goes down.
	void handle_mouse_button(bool is_down, const r4::vector2<float>& pos, ruis::mouse_button button, unsigned id)
	{
		this->gui.send_mouse_button(is_down, pos, button, id);
	}

	friend void handle_mouse_button(
		application& app,
		bool is_down,
		const r4::vector2<float>& pos,
		ruis::mouse_button button,
		unsigned id
	);

	void handle_mouse_hover(bool is_hovered, unsigned id)
	{
		this->gui.send_mouse_hover(is_hovered, id);
	}

	friend void handle_mouse_hover(application& app, bool is_hovered, unsigned pointer_id);

protected:
	/**
	 * @brief Application constructor.
	 * @param name - name of the application.
	 * @param requested_window_params - requested window parameters.
	 */
	application(std::string name, const window_params& requested_window_params);

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
	void show_virtual_keyboard() noexcept;

	/**
	 * @brief Hide virtual keyboard.
	 * On mobile platforms this function hides the on-screen keyboard.
	 * On desktop platforms this function does nothing.
	 */
	void hide_virtual_keyboard() noexcept;

private:
	// The idea with unicode_resolver parameter is that we don't want to calculate
	// the unicode unless it is really needed, thus postpone it as much as
	// possible.
	void handle_character_input(const ruis::gui::input_string_provider& string_provider, ruis::key key_code)
	{
		this->gui.send_character_input(string_provider, key_code);
	}

	friend void handle_character_input(
		application& app,
		const ruis::gui::input_string_provider& string_provider,
		ruis::key key_code
	);

	void handle_key_event(bool is_down, ruis::key key_code);

	friend void handle_key_event(application& app, bool is_down, ruis::key key_code);

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
	bool is_fullscreen_v = false;

	r4::rectangle<int> before_fullscreen_window_rect{0, 0, 0, 0};

public:
	/**
	 * @brief Check if application currently runs in fullscreen mode.
	 * @return true if application is in fullscreen mode.
	 * @return false if application is in windowed mode.
	 */
	bool is_fullscreen() const noexcept
	{
		return this->is_fullscreen_v;
	}

	/**
	 * @brief Set/unset fullscreen mode.
	 * @param enable - whether to enable or to disable fullscreen mode.
	 */
	void set_fullscreen(bool enable);

	/**
	 * @brief Show/hide mouse cursor.
	 * @param visible - whether to show (true) or hide (false) mouse cursor.
	 */
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
		r4::vector2<unsigned> screen_size_pixels,
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
	using factory_type = std::function<std::unique_ptr<application>(utki::span<const char*>)>;

	/**
	 * @brief Constructor.
	 * Registers the application object factory function.
	 * Only one application factory can be registered.
	 * @param factory - application factory function.
	 * @throw std::logic_error - in case a factory is already registered.
	 */
	application_factory(factory_type&& factory);

	static const factory_type& get_factory();

private:
	static factory_type& get_factory_internal();
};

} // namespace ruisapp
