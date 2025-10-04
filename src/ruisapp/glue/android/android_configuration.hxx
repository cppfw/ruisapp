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

#include <android/configuration.h>

namespace {
class android_configuration_wrapper
{
	AConfiguration* config;

public:
	explicit android_configuration_wrapper(AAssetManager& am);

	android_configuration_wrapper(const android_configuration_wrapper&) = delete;
	android_configuration_wrapper& operator=(const android_configuration_wrapper&) = delete;

	android_configuration_wrapper(android_configuration_wrapper&&) = delete;
	android_configuration_wrapper& operator=(android_configuration_wrapper&&) = delete;

	~android_configuration_wrapper();

	int32_t diff(const android_configuration_wrapper& cfg);

	int32_t get_orientation() const noexcept;
};
} // namespace
