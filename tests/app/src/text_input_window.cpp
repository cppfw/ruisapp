#include "text_input_window.hpp"

#include <ruis/widget/button/tab_group.hpp>
#include <ruis/widget/button/tab.hpp>
#include <ruis/widget/button/impl/check_box.hpp>
#include <ruis/widget/group/collapse_area.hpp>
#include <ruis/widget/button/impl/image_push_button.hpp>
#include <ruis/widget/label/gap.hpp>
#include <ruis/widget/input/text_input_line.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace ruis::length_literals;

namespace m{
using namespace ruis::make;
}

namespace{
utki::shared_ref<ruis::push_button> make_push_button(
    utki::shared_ref<ruis::context> c, //
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
}

utki::shared_ref<ruis::window> make_text_input_window(
    utki::shared_ref<ruis::context> c,
    ruis::vec2_length pos
)
{
    auto vsync_check_box = m::check_box(c, {
        .button_params{
            .pressed = c.get().ren().ctx().is_vsync_enabled()
        }
    });
    vsync_check_box.get().pressed_change_handler = [](ruis::button& b){
        b.context.get().ren().ctx().set_vsync_enabled(b.is_pressed());
    };

    // clang-format off
    return m::window(c,
        {
            .widget_params{
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
            .container_params{
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
                            .widget_params{
                                .id = "showhide_mousecursor_button"s
                            }
                        },
                        {
                            m::text(c, {}, U"show/hide mouse"s)
                        }
                    ),
                    m::push_button(c,
                        {
                            .widget_params{
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
            m::nine_patch(c,
                {
                    .layout_params{
                        .dims{ruis::dim::max, ruis::dim::min}
                    },
                    .widget_params{
                        .id = "text_input"s
                    },
                    .nine_patch_params{
                        .nine_patch = c.get().loader().load<ruis::res::nine_patch>("ruis_npt_textfield_background"sv)
                    }
                },
                {
                    m::text_input_line(c,
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
