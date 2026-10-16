#pragma once

#include <ruis/style/styled.hpp>
#include <ruis/util/length.hpp>
#include <ruis/util/sides.hpp>
#include <ruis/widget/widget.hpp>

/**
 * @brief Create the root widget of the test window.
 * @param c - ruis context.
 * @param insets - the live content insets of the window (see
 *        ruisapp::window::get_insets()). They are applied as borders of a
 *        padding widget so that the padded area shrinks as the system bars /
 *        the on-screen keyboard cover the window. The insets are live, so they
 *        are updated without re-querying.
 */
utki::shared_ref<ruis::widget> make_root_widget(
	const utki::shared_ref<ruis::context>& c, //
	const ruis::sides<ruis::styled<ruis::length>>& insets
);
