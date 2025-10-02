#pragma once

namespace {
class java_functions_wrapper
{
	JNIEnv* env;
	jclass clazz;
	jobject obj;

	jmethodID resolve_key_unicode_method;

	jmethodID get_dots_per_inch_method;
	jmethodID get_screen_dims_method;

	jmethodID show_virtual_keyboard_method;
	jmethodID hide_virtual_keyboard_method;

	jmethodID list_dir_contents_method;

	jmethodID get_storage_dir_method;

public:
	java_functions_wrapper();

	char32_t resolve_key_unicode(
		int32_t dev_id, //
		int32_t meta_state,
		int32_t key_code
	);

	float get_dots_per_inch();
	r4::vector2<unsigned> get_screen_dims();

	void hide_virtual_keyboard();
	void show_virtual_keyboard();

	std::vector<std::string> list_dir_contents(const std::string& path);
	std::string get_storage_dir();
};
} // namespace
