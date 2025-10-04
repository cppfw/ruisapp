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

namespace {
class java_functions_wrapper
{
	JNIEnv* env;
	jclass clazz;
	jobject obj;

	jmethodID resolve_key_unicode_method;

	jmethodID get_dots_per_inch_method;
	jmethodID get_screen_resolution_method;

	jmethodID show_virtual_keyboard_method;
	jmethodID hide_virtual_keyboard_method;

	jmethodID list_dir_contents_method;

	jmethodID get_storage_dir_method;

public:
	java_functions_wrapper();

	char32_t resolve_key_unicode(
		int32_t dev_id, //
		int32_t meta_state,
		int32_t key_code
	);

	float get_dots_per_inch();
	r4::vector2<unsigned> get_screen_resolution();

	void hide_virtual_keyboard();
	void show_virtual_keyboard();

	std::vector<std::string> list_dir_contents(const std::string& path);
	std::string get_storage_dir();
};
} // namespace
