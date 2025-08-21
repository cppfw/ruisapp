
namespace {
class xorg_display_wrapper
{
	Display* display_v;

public:
	xorg_display_wrapper() :
		display_v([]() {
			auto d = XOpenDisplay(nullptr);
			if (!d) {
				throw std::runtime_error("XOpenDisplay() failed");
			}
			return d;
		}())
	{}

	xorg_display_wrapper(const xorg_display_wrapper&) = delete;
	xorg_display_wrapper& operator=(const xorg_display_wrapper&) = delete;

	xorg_display_wrapper(xorg_display_wrapper&&) = delete;
	xorg_display_wrapper& operator=(xorg_display_wrapper&&) = delete;

	Display* display()
	{
		return this->display_v;
	}

	~xorg_display_wrapper()
	{
		XCloseDisplay(this->display_v);
	}
};
} // namespace
