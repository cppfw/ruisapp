namespace {

class xorg_window_wrapper : public ruisapp::window
{
	utki::shared_ref<display_wrapper> display;

#ifdef RUISAPP_RENDER_OPENGL
	struct xorg_fb_config_wrapper {
		GLXFBConfig fb_config;

		xorg_fb_config_wrapper(
			display_wrapper& display, //
			const ruisapp::window_parameters& window_params
		) :
			fb_config([&]() {
				GLXFBConfig best_fb_config = nullptr;
				std::vector<int> visual_attribs;
				visual_attribs.push_back(GLX_X_RENDERABLE);
				visual_attribs.push_back(True);
				visual_attribs.push_back(GLX_X_VISUAL_TYPE);
				visual_attribs.push_back(GLX_TRUE_COLOR);
				visual_attribs.push_back(GLX_DRAWABLE_TYPE);
				visual_attribs.push_back(GLX_WINDOW_BIT);
				visual_attribs.push_back(GLX_RENDER_TYPE);
				visual_attribs.push_back(GLX_RGBA_BIT);
				visual_attribs.push_back(GLX_DOUBLEBUFFER);
				visual_attribs.push_back(True);
				visual_attribs.push_back(GLX_RED_SIZE);
				visual_attribs.push_back(utki::byte_bits);
				visual_attribs.push_back(GLX_GREEN_SIZE);
				visual_attribs.push_back(utki::byte_bits);
				visual_attribs.push_back(GLX_BLUE_SIZE);
				visual_attribs.push_back(utki::byte_bits);
				visual_attribs.push_back(GLX_ALPHA_SIZE);
				visual_attribs.push_back(utki::byte_bits);

				if (window_params.buffers.get(ruisapp::buffer::depth)) {
					visual_attribs.push_back(GLX_DEPTH_SIZE);
					visual_attribs.push_back(utki::byte_bits * 3); // 24 bits per pixel for depth buffer
				}
				if (window_params.buffers.get(ruisapp::buffer::stencil)) {
					visual_attribs.push_back(GLX_STENCIL_SIZE);
					visual_attribs.push_back(utki::byte_bits);
				}

				visual_attribs.push_back(None);

				utki::span<GLXFBConfig> fb_configs = [&]() {
					int fbcount = 0;
					GLXFBConfig* fb_configs = glXChooseFBConfig(
						display.display(),
						display.get_default_screen(),
						visual_attribs.data(),
						&fbcount
					);
					if (!fb_configs) {
						throw std::runtime_error("glXChooseFBConfig() returned empty list");
					}
					return utki::make_span(fb_configs, fbcount);
				}();

				utki::scope_exit scope_exit_fbc([&fb_configs]() {
					// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
					XFree(fb_configs.data());
				});

				GLXFBConfig worst_fb_config = nullptr;
				int best_num_samp = -1;
				// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
				int worst_num_samp = 999;

				for (auto fb_config : fb_configs) {
					XVisualInfo* vi = glXGetVisualFromFBConfig(display.display(), fb_config);
					if (!vi) {
						continue;
					}

					// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
					int samp_buf;
					glXGetFBConfigAttrib(display.display(), fb_config, GLX_SAMPLE_BUFFERS, &samp_buf);

					// NOLINTNEXTLINE(cppcoreguidelines-init-variables)
					int samples;
					glXGetFBConfigAttrib(display.display(), fb_config, GLX_SAMPLES, &samples);

					if (!best_fb_config || (samp_buf && samples > best_num_samp)) {
						best_fb_config = fb_config;
						best_num_samp = samples;
					}
					if (!worst_fb_config || !samp_buf || samples < worst_num_samp) {
						worst_fb_config = fb_config;
						worst_num_samp = samples;
					}

					XFree(vi);
				}

				utki::assert(best_fb_config, SL);
				return best_fb_config;
			}())
		{}

		// no need to free fb config
		~xorg_fb_config_wrapper() = default;
	} xorg_fb_config;
#endif

	Colormap color_map;
	::Window window;

	ruis::real scale_factor = 1;

public:
	xorg_window_wrapper(
		utki::shared_ref<display_wrapper> display, //
		const ruisapp::window_parameters& window_params
	) :
		display(std::move(display))
#ifdef RUISAPP_RENDER_OPENGL
		,
		xorg_fb_config(
			this->display, //
			window_params
		)
#endif
	{}

	// TODO: remove
#ifdef RUISAPP_RENDER_OPENGL
	GLXFBConfig& fb_config()
	{
		return this->xorg_fb_config.fb_config;
	}
#endif
};

} // namespace
