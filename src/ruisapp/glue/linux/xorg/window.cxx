namespace {

class xorg_window_wrapper : public ruisapp::window
{
	utki::shared_ref<xorg_display_wrapper> display;

public:
	xorg_window_wrapper(utki::shared_ref<xorg_display_wrapper> display) :
		display(std::move(display))
	{}
};

} // namespace
