#include "android_configuration.hxx"

android_configuration_wrapper::android_configuration_wrapper(AAssetManager& am) :
	config(AConfiguration_new())
{
	AConfiguration_fromAssetManager(
		this->config, //
		&am
	);
}

android_configuration_wrapper::~android_configuration_wrapper()
{
	AConfiguration_delete(this->config);
}

int32_t android_configuration_wrapper::diff(const android_configuration_wrapper& cfg)
{
	return AConfiguration_diff(
		this->config, //
		cfg.config
	);
}

int32_t android_configuration_wrapper::get_orientation() const noexcept
{
	return AConfiguration_getOrientation(this->config);
}
