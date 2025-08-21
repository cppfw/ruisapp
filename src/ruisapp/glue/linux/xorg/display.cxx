
namespace {
class xorg_display_wrapper
{
	Display* display_v;

	struct xorg_input_method_wrapper {
		XIM xim;

		xorg_input_method_wrapper(Display* display)
		{
			this->xim = XOpenIM(
				display, //
				nullptr,
				nullptr,
				nullptr
			);
			if (this->xim == nullptr) {
				throw std::runtime_error("XOpenIM() failed");
			}
		}

		~xorg_input_method_wrapper()
		{
			XCloseIM(this->xim);
		}
	} input_method_v;

public:
	xorg_display_wrapper() :
		display_v([]() {
			auto d = XOpenDisplay(nullptr);
			if (!d) {
				throw std::runtime_error("XOpenDisplay() failed");
			}
			return d;
		}()),
        input_method_v(this->display_v)
	{}

	xorg_display_wrapper(const xorg_display_wrapper&) = delete;
	xorg_display_wrapper& operator=(const xorg_display_wrapper&) = delete;

	xorg_display_wrapper(xorg_display_wrapper&&) = delete;
	xorg_display_wrapper& operator=(xorg_display_wrapper&&) = delete;

	Display* display()
	{
		return this->display_v;
	}

    XIM& input_method(){
        return this->input_method_v.xim;
    }

	void flush()
	{
		XFlush(this->display_v);
	}

	~xorg_display_wrapper()
	{
		XCloseDisplay(this->display_v);
	}
};
} // namespace
