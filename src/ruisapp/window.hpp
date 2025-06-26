#pragma once

#include <r4/vector.hpp>
#include <utki/flags.hpp>

namespace ruisapp {

/**
 * @brief Desired window parameters.
 */
struct window_params {
	/**
	 * @brief Desired dimensions of the window
	 */
	r4::vector2<unsigned> dims;

	/**
	 * @brief Window title.
	 */
	std::string title = "ruisapp";

	/**
	 * @brief Graphics buffer kind.
	 * Color buffer is always present, so no enum entry for color buffer is needed.
	 */
	enum class buffer {
		depth,
		stencil,

		enum_size
	};

	/**
	 * @brief Flags describing desired buffers for rendering context.
	 * Color buffer is always there implicitly.
	 */
	utki::flags<buffer> buffers = false;

	// version 0.0 means default version
	// clang-format off
	utki::version_duplet graphics_api_version = {
		.major = 0,
		.minor = 0
	};
	// clang-format on

	window_params(r4::vector2<unsigned> dims) :
		dims(dims)
	{}
};

class window
{
public:
};

} // namespace ruisapp
