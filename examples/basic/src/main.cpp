#include <mordavokne/application.hpp>

#include <morda/widgets/label/TextLabel.hpp>
#include <morda/widgets/button/Button.hpp>


class Application : public mordavokne::application{
public:
	Application() :
			mordavokne::App(mordavokne::window_params(r4::vec2ui(800, 600)))
	{
		morda::Morda::inst().initStandardWidgets(*this->get_res_file());

		//Inflate widgets hierarchy from GUI description script
		auto c = morda::Morda::inst().inflater.inflate(
				*this->get_res_file("res/main.gui")
			);

		//set the widgets hierarchy to the application
		morda::Morda::inst().setRootWidget(c);

		auto textLabel = c->findChildByNameAs<morda::TextLabel>("info_text");
		ASSERT(textLabel)

		auto button = c->findChildByNameAs<morda::PushButton>("hw_button");

		auto textLabelWeak = utki::make_weak(textLabel);//make a weak pointer to the TextLabel widget.

		bool even = true;

		//connect some action on button click
		button->clicked = [textLabelWeak, even](morda::PushButton&) mutable {
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



std::unique_ptr<mordavokne::application> mordavokne::create_application(int argc, const char** argv, const utki::Buf<std::uint8_t> savedState){
	return utki::makeUnique<Application>();
}

