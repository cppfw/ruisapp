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

#include "../application.hpp"

namespace{

std::unique_ptr<mordavokne::application> createAppUnix(int argc, const char** argv){
	return mordavokne::application_factory::get_factory()(utki::make_span(argv, argc));
}

std::string initialize_storage_dir(const std::string& appName){
	auto homeDir = getenv("HOME");
	if(!homeDir){
		throw std::runtime_error("failed to get user home directory. Is HOME environment variable set?");
	}

	std::string homeDirStr(homeDir);

	if(*homeDirStr.rend() != '/'){
		homeDirStr.append(1, '/');
	}

	homeDirStr.append(1, '.').append(appName).append(1, '/');

	papki::fs_file dir(homeDirStr);
	if(!dir.exists()){
		dir.make_dir();
	}

	return homeDirStr;
}

}
