#include "root_widget.hpp"

#include <ruis/widget/proxy/key_proxy.hpp>
#include <ruis/widget/group/overlay.hpp>
#include <ruis/widget/label/image_mouse_cursor.hpp>
#include <ruis/widget/label/image.hpp>

#include "window1.hpp"
#include "sliders_window.hpp"
#include "spinning_cube_window.hpp"
#include "text_input_window.hpp"
#include "scroll_area_window.hpp"
#include "gradient_window.hpp"
#include "vertical_list_window.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace ruis::length_literals;

namespace m{
using namespace ruis::make;
}

utki::shared_ref<ruis::widget> make_root_widget(utki::shared_ref<ruis::context> c){
    // clang-format off
    return m::key_proxy(c,
        {},
        {
            m::image_mouse_cursor(c,
                {
                    .layout_params{
                        .dims{ruis::dim::fill, ruis::dim::fill}
                    },
                    .mouse_cursor_params{
                        .cursor = c.get().loader().load<ruis::res::cursor>("crs_arrow"sv)
                    }
                },
                {
                    m::overlay(c,
                        {
                            .layout_params{
                                .dims{ruis::dim::fill, ruis::dim::fill}
                            },
                            .widget_params{
                                .id = "overlay"s
                            },
                        },
                        {
                            m::image(c,
                                {
                                    .layout_params{
                                        .dims = {ruis::dim::fill, ruis::dim::fill}
                                    },
                                    .image_params{
                                        .img = c.get().loader().load<ruis::res::image>("img_sample")
                                    }
                                }
                            ),
                            m::container(c,
                                {
                                    .layout_params{
                                        .dims{ruis::dim::fill, ruis::dim::fill}
                                    },
                                    .container_params{
                                        .layout = ruis::layout::trivial
                                    }
                                },
                                {
                                    make_window1(c, {300_pp, 10_pp}),
                                    make_sliders_window(c, {0_pp, 250_pp}),
                                    make_spinning_cube_window(c, {10_pp, 500_pp}),
                                    make_text_input_window(c, {310_pp, 500_pp}),
                                    make_scroll_area_window(c, {620_pp, 500_pp}),
                                    make_gradient_window(c, {620_pp, 250_pp}),
                                    make_vertical_list_window(c, {620_pp, 0_pp})
                                }
                            )
                        }
                    )
                }
            )
        }
    );
    // clang-format on
}
