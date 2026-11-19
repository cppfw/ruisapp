#include "text_input_window.hpp"

#include <ruis/widget/button/impl/check_box.hpp>
#include <ruis/widget/button/impl/image_push_button.hpp>
#include <ruis/widget/button/tab.hpp>
#include <ruis/widget/button/tab_group.hpp>
#include <ruis/widget/group/collapse_area.hpp>
#include <ruis/widget/input/text_input.hpp>
#include <ruis/widget/label/gap.hpp>
#include <ruisapp/application.hpp>

#include "new_native_window.hpp"
#include "style.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace ruis::length_literals;

namespace {
utki::shared_ref<ruis::push_button> make_push_button(
	const utki::shared_ref<ruis::context>& c, //
	std::u32string text
)
{
	// clang-format off
	return m::push_button(c,
		{
			.layout_params{
				.dims{ruis::dim::min, ruis::dim::fill},
				.weight = 1
			}
		},
		{
			m::text(c,
				{},
				std::move(text)
			)
		}
	);
	// clang-format on
}
} // namespace

utki::shared_ref<ruis::window> make_text_input_window(const utki::shared_ref<ruis::context>& c, ruis::vec2_length pos)
{
	// clang-format off
	auto vsync_check_box = m::check_box(c,
		{
			.button{
				.pressed = c.get().ren().ctx().is_vsync_enabled()
			}
		}
	);
	// clang-format on
	vsync_check_box.get().pressed_change_handler = [](ruis::button& b) {
		b.context.get().ren().ctx().set_vsync_enabled(b.is_pressed());
	};

	// clang-format off
	auto new_native_window_button = m::push_button(c,
		{
			.widget{
				.id = "new_native_window_button"s
			}
		},
		{
			m::text(c, {}, U"new window"s)
		}
	);
	// clang-format on

	new_native_window_button.get().click_handler = [](ruis::push_button& b) {
		utki::logcat("new native window button clicked", '\n');

		auto& nw = ruisapp::inst().make_window({.title = "new native_window"s, .taskbar = false});

		auto c = make_new_native_window_root_widget(
			nw.gui.context, //
			nw
		);
		nw.gui.set_root(c);

		// the application is responsible for setting the window's close handler;
		// when the window manager requests closing the window (e.g. via the
		// title bar close button) we destroy it
		{
			auto* nw_ptr = &nw;
			nw.gui.context.get().window().close_handler = [nw_ptr]() {
				ruisapp::inst().destroy_window(*nw_ptr);
			};
		}
	};

	// clang-format off
	return m::window(c,
		{
			.widget{
				.rectangle{
					{
						pos.x().get(c.get()),
						pos.y().get(c.get())
					},
					{
						ruis::length::make_pp(300).get(c.get()),
						ruis::length::make_pp(200).get(c.get())
					}
				}
			},
			.params{
				.layout = ruis::layout::column
			},
			.title = U"TextInput"s
		},
		{
			m::row(c,
				{
					.layout_params{
						.align{ruis::align::front, ruis::align::front}
					}
				},
				{
					m::push_button(c,
						{
							.widget{
								.id = "showhide_mousecursor_button"s
							}
						},
						{
							m::text(c, {}, U"show/hide mouse"s)
						}
					),
					m::push_button(c,
						{
							.widget{
								.id = "fullscreen_button"s
							}
						},
						{
							m::text(c, {}, U"toggle fullscreen"s)
						}
					),
					vsync_check_box,
					m::text(c, {}, U"VSYNC"s)
				}
			),
			m::row(c,
				{
					.layout_params{
						.align{ruis::align::front, ruis::align::front}
					}
				},
				{
					std::move(new_native_window_button)
				}
			),
			m::nine_patch(c,
				{
					.layout_params{
						.dims{ruis::dim::max, ruis::dim::min}
					},
					.widget{
						.id = "text_input"s
					},
					.params{
						.specific{
							.source = c.get().loader().load<ruis::res::nine_patch>("ruis_npt_textfield_background"sv)
						}
					}
				},
				{
					m::text_input(c,
						{
							.layout_params{
								.dims{ruis::dim::fill, ruis::dim::max}
							}
						},
						U"Hello Wrodl!!!"s
					)
				}
			),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s),
			make_push_button(c, U"button!!!"s)
		}
	);
	// clang-format on
}
