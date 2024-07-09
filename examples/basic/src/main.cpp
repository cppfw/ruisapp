#include <ruisapp/application.hpp>

#include <ruis/widget/label/TextLabel.hpp>
#include <ruis/widget/button/Button.hpp>


class Application : public ruisapp::application{
public:
	Application() :
			ruisapp::App(ruisapp::window_params(r4::vector2<unsigned>(800, 600)))
	{
		ruis::Morda::inst().init_standard_widgets(*this->get_res_file());

		//Inflate widgets hierarchy from GUI description script
		auto c = ruis::Morda::inst().inflater.inflate(
				*this->get_res_file("res/main.gui")
			);

		//set the widgets hierarchy to the application
		ruis::Morda::inst().setRootWidget(c);

		auto textLabel = c->findChildByNameAs<ruis::TextLabel>("info_text");
		ASSERT(textLabel)

		auto button = c->findChildByNameAs<ruis::PushButton>("hw_button");

		auto textLabelWeak = utki::make_weak(textLabel);//make a weak pointer to the TextLabel widget.

		bool even = true;

		//connect some action on button click
		button->clicked = [textLabelWeak, even](ruis::PushButton&) mutable {
			if(auto tl = textLabelWeak.lock()){
				even = !even;
				if(even){
					tl->setText("even");
				}else{
					tl->setText("odd");
				}
			}
		};
	}
};



std::unique_ptr<ruisapp::application> ruisapp::create_application(int argc, const char** argv, const utki::Buf<uint8_t> savedState){
	return utki::makeUnique<Application>();
}

