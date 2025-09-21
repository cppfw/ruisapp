#pragma once

#include <utki/windows.hpp>

namespace {
class display_wrapper {
public:
struct window_class_wrapper {
	const char* const window_class_name;

	window_class_wrapper(
		const char* window_class_name,
		WNDPROC window_procedure
	);

	window_class_wrapper(const window_class_wrapper&) = delete;
	window_class_wrapper& operator=(const window_class_wrapper) = delete;

	window_class_wrapper(window_class_wrapper&&) = delete;
	window_class_wrapper& operator=(window_class_wrapper&&) = delete;

	~window_class_wrapper();
};

	window_class_wrapper dummy_window_class;
	window_class_wrapper regular_window_class;

	display_wrapper();
};
}
