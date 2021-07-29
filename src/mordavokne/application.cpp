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

#include "application.hpp"

#include <utki/debug.hpp>
#include <utki/config.hpp>

#include <papki/fs_file.hpp>
#include <papki/root_dir.hpp>

using namespace mordavokne;

application::T_Instance application::instance;

void application::render(){
	//TODO: render only if needed?
	this->gui.context->renderer->clear_framebuffer();

	this->gui.render(this->gui.context->renderer->initial_matrix);

	this->swap_frame_buffers();
}

void application::update_window_rect(const morda::rectangle& rect){
	if(this->curWinRect == rect){
		return;
	}

	this->curWinRect = rect;

	LOG([&](auto&o){o << "application::update_window_rect(): this->curWinRect = " << this->curWinRect << std::endl;})
	this->gui.context->renderer->set_viewport(r4::rectangle<int>(
			int(this->curWinRect.p.x()),
			int(this->curWinRect.p.y()),
			int(this->curWinRect.d.x()),
			int(this->curWinRect.d.y())
		));

	this->gui.set_viewport(this->curWinRect.d);
}

#if M_OS_NAME != M_OS_NAME_ANDROID && M_OS_NAME != M_OS_NAME_IOS
std::unique_ptr<papki::file> application::get_res_file(const std::string& path)const{
	return std::make_unique<papki::fs_file>(path);
}

void application::show_virtual_keyboard()noexcept{
	LOG([](auto&o){o << "application::show_virtual_keyboard(): invoked" << std::endl;})
	// do nothing
}

void application::hide_virtual_keyboard()noexcept{
	LOG([](auto&o){o << "application::hide_virtual_keyboard(): invoked" << std::endl;})
	// do nothing
}
#endif

morda::real application::get_pixels_per_dp(r4::vector2<unsigned> resolution, r4::vector2<unsigned> screenSizeMm){

	// NOTE: for ordinary desktop displays the PT size should be equal to 1 pixel.
	// For high density displays it should be more than one pixel, depending on display ppi.
	// For hand held devices the size of PT should be determined from physical screen size and pixel resolution.

#if M_OS_NAME == M_OS_NAME_IOS
	return morda::real(1); // TODO:
#else
	unsigned xIndex;
	if(resolution.x() > resolution.y()){
		xIndex = 0;
	}else{
		xIndex = 1;
	}

	if(screenSizeMm[xIndex] < 300){
		return resolution[xIndex] / morda::real(700);
	}else if(screenSizeMm[xIndex] < 150) {
        return resolution[xIndex] / morda::real(200);
    }

	return morda::real(1);
#endif
}

application_factory::factory_type& application_factory::get_factory_internal(){
	static application_factory::factory_type f;
	return f;
}

const application_factory::factory_type& application_factory::get_factory(){
	auto& f = get_factory_internal();
	if(!f){
		throw std::logic_error("no application factory registered");
	}
	return f;
}

application_factory::application_factory(factory_type&& factory){
	auto& f = this->get_factory_internal();
	if(f){
		throw std::logic_error("application factory is already registered");
	}
	f = std::move(factory);
}
