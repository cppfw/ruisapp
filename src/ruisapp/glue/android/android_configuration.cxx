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

#include "android_configuration.hxx"

android_configuration_wrapper::android_configuration_wrapper(AAssetManager& am) :
	config(AConfiguration_new())
{
	AConfiguration_fromAssetManager(
		this->config, //
		&am
	);
}

android_configuration_wrapper::~android_configuration_wrapper()
{
	AConfiguration_delete(this->config);
}

int32_t android_configuration_wrapper::diff(const android_configuration_wrapper& cfg)
{
	return AConfiguration_diff(
		this->config, //
		cfg.config
	);
}

int32_t android_configuration_wrapper::get_orientation() const noexcept
{
	return AConfiguration_getOrientation(this->config);
}
