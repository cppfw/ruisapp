namespace {

class xorg_window_wrapper : public ruisapp::window
{
	utki::shared_ref<display_wrapper> display;

	Colormap color_map;
	::Window window;

	ruis::real scale_factor = 1;

public:
	xorg_window_wrapper(utki::shared_ref<display_wrapper> display) :
		display(std::move(display))
	{}
};

} // namespace
