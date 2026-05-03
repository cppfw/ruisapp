#include "scroll_area_window.hpp"

#include <ruis/widget/group/scroll_area.hpp>
#include <ruis/widget/group/collapse_area.hpp>
#include <ruis/widget/label/image.hpp>
#include <ruis/widget/button/push_button.hpp>
#include <ruis/widget/slider/scroll_bar.hpp>
#include <ruis/widget/proxy/min_proxy.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace ruis::length_literals;

namespace m{
using namespace ruis::make;
}

// In GCC 12.2 with -O3 and -g3, the GCC compiler complains:
// std::__cxx11::basic_string<char>::_Alloc_hider::_M_p' may be used uninitialized [-Werror=maybe-uninitialized]
// So, we suppress the warning.
#if CFG_COMPILER == CFG_COMPILER_GCC && CFG_COMPILER_VERSION_MAJOR == 12 && CFG_COMPILER_VERSION_MINOR == 2
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

utki::shared_ref<ruis::window> make_scroll_area_window(
    utki::shared_ref<ruis::context> c, //
    ruis::vec2_length pos
)
{
    // clang-format off
    return m::window(c,
        {
            .widget_params{
                .id = "scroll_area_root_container"s,
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
            .title = U"ScrollArea"s
        },
        {
            m::row(c,
                {
                    .layout_params{
                        .dims{ruis::dim::max, ruis::dim::fill},
                        .weight = 1
                    },
                    .widget_params{
                        .id = "scroll_area_subcontainer"s
                    }
                },
                {
                    m::scroll_area(c,
                        {
                            .layout_params{
                                .dims{ruis::dim::fill, ruis::dim::max},
                                .weight = 1
                            },
                            .widget_params{
                                .id = "scroll_area"s,
                                .clip = true
                            }
                        },
                        {
                            m::image(c,
                                {
                                    .layout_params{
                                        .dims{ruis::dim::min, ruis::dim::max}
                                    },
                                    .image_params{
                                        .img = c.get().loader().load<ruis::res::image>("img_sample"sv)
                                    }
                                }
                            ),
                            m::collapse_area(c,
                                {
                                    .widget_params{
                                        .rectangle{
                                            {
                                                (20_pp).get(c.get()),
                                                (450_pp).get(c.get())
                                            }, {}
                                        }
                                    },
                                    .title = U"Collapsable stuff"s
                                },
                                {
                                    m::text(c,{}, U"I'm collapsable!!!"s)
                                }
                            ),
                            m::push_button(c,
                                {
                                    .widget_params{
                                        .id = "push_button_in_scroll_container"s,
                                        .rectangle{
                                            {
                                                (10_mm).get(c.get()),
                                                (20_mm).get(c.get())
                                            },
                                            {}
                                        }
                                    }
                                },
                                {
                                    m::text(c, {}, U"Hello World!!!"s)
                                }
                            )
                        }
                    ),
                    m::scroll_bar(c,
                        {
                            .layout_params{
                                .dims{ruis::dim::min, ruis::dim::max}
                            },
                            .widget_params{
                                .id = "scroll_area_vertical_slider"s
                            },
                            .oriented_params{
                                .vertical = true
                            }
                        }
                    )
                }
            ),
            m::row(c,
                {
                    .layout_params{
                        .dims{ruis::dim::max, ruis::dim::min}
                    }
                },
                {
                    m::scroll_bar(c,
                        {
                            .layout_params{
                                .dims{ruis::dim::fill, ruis::dim::min},
                                .weight = 1
                            },
                            .widget_params{
                                .id = "scroll_area_horizontal_slider"s
                            },
                            .oriented_params{
                                .vertical = false
                            }
                        }
                    ),
                    m::min_proxy(c,
                        {
                            .layout_params{
                                .dims{ruis::dim::min, ruis::dim::fill}
                            },
                            .min_proxy_params{
                                .root_id = "scroll_area_root_container"s,
                                .target_id_path = {
                                    "scroll_area_subcontainer"s,
                                    "scroll_area_vertical_slider"s
                                }
                            }
                        }
                    )
                }
            )
        }
    );
    // clang-format on
}

#if CFG_COMPILER == CFG_COMPILER_GCC && CFG_COMPILER_VERSION_MAJOR == 12 && CFG_COMPILER_VERSION_MINOR == 2
#	pragma GCC diagnostic pop
#endif
