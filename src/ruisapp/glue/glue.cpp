/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2024  Ivan Gagis <igagis@gmail.com>

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

#include <utki/config.hpp>

#if CFG_OS == CFG_OS_WINDOWS
// NOLINTNEXTLINE(bugprone-suspicious-include)
#	include "windows/glue.cxx"
#elif CFG_OS == CFG_OS_LINUX && CFG_OS_NAME == CFG_OS_NAME_ANDROID
#	include "android/glue.cxx"
#elif CFG_OS == CFG_OS_LINUX
#   ifdef RUISAPP_WAYLAND
// NOLINTNEXTLINE(bugprone-suspicious-include)
#       include "linux/glue_wayland.cxx"
#   else
// NOLINTNEXTLINE(bugprone-suspicious-include)
#	    include "linux/glue_x11.cxx"
#   endif
#endif
