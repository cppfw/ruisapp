#pragma once

#include <android/configuration.h>

namespace {
class android_configuration_wrapper
{
	AConfiguration* config;

public:
	explicit android_configuration_wrapper(AAssetManager& am);

	android_configuration_wrapper(const android_configuration_wrapper&) = delete;
	android_configuration_wrapper& operator=(const android_configuration_wrapper&) = delete;

	android_configuration_wrapper(android_configuration_wrapper&&) = delete;
	android_configuration_wrapper& operator=(android_configuration_wrapper&&) = delete;

	~android_configuration_wrapper();

	int32_t diff(const android_configuration_wrapper& cfg);

	int32_t get_orientation() const noexcept;
};
} // namespace
