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

namespace mordavokne{

const decltype(application::window_pimpl)& get_window_pimpl(application& app){
	return app.window_pimpl;
}

void render(application& app){
	app.render();
}

void update_window_rect(application& app, const morda::rectangle& rect){
	app.update_window_rect(rect);
}

void handle_mouse_move(application& app, const r4::vector2<float>& pos, unsigned id){
	app.handle_mouse_move(pos, id);
}

void handle_mouse_button(application& app, bool isDown, const r4::vector2<float>& pos, morda::mouse_button button, unsigned id){
	app.handle_mouse_button(isDown, pos, button, id);
}

void handleMouseHover(application& app, bool isHovered, unsigned pointerID){
	app.handleMouseHover(isHovered, pointerID);
}

void handle_character_input(application& app, const morda::gui::unicode_provider& unicode_resolver, morda::key key_code){
	app.handle_character_input(unicode_resolver, key_code);
}

void handle_key_event(application& app, bool isDown, morda::key keyCode){
	app.handle_key_event(isDown, keyCode);
}

}
