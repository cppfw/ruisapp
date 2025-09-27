#pragma once

namespace {
class java_functions_wrapper
{
	JNIEnv* env;
	jclass clazz;
	jobject obj;

	jmethodID resolve_key_unicode_method;

	jmethodID get_dots_per_inch_method;

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
	)
	{
		return char32_t(this->env->CallIntMethod(
			this->obj,
			this->resolve_key_unicode_method,
			jint(dev_id),
			jint(meta_state),
			jint(key_code)
		));
	}

	float get_dots_per_inch()
	{
		return float(this->env->CallFloatMethod(
			this->obj, //
			this->get_dots_per_inch_method
		));
	}

	void hide_virtual_keyboard()
	{
		this->env->CallVoidMethod(
			this->obj, //
			this->hide_virtual_keyboard_method
		);
	}

	void show_virtual_keyboard()
	{
		this->env->CallVoidMethod(
			this->obj, //
			this->show_virtual_keyboard_method
		);
	}

	std::vector<std::string> list_dir_contents(const std::string& path)
	{
		jstring p = this->env->NewStringUTF(path.c_str());
		jobject res = this->env->CallObjectMethod(this->obj, this->list_dir_contents_method, p);
		this->env->DeleteLocalRef(p);

		utki::scope_exit scopeExit([this, res]() {
			this->env->DeleteLocalRef(res);
		});

		std::vector<std::string> ret;

		if (res == nullptr) {
			return ret;
		}

		jobjectArray arr = static_cast<jobjectArray>(res);

		int count = env->GetArrayLength(arr);

		for (int i = 0; i < count; ++i) {
			jstring str = static_cast<jstring>(env->GetObjectArrayElement(arr, i));
			auto chars = env->GetStringUTFChars(str, nullptr);
			ret.push_back(std::string(chars));
			this->env->ReleaseStringUTFChars(str, chars);
			this->env->DeleteLocalRef(str);
		}

		return ret;
	}

	std::string get_storage_dir()
	{
		jobject res = this->env->CallObjectMethod(this->obj, this->get_storage_dir_method);
		utki::scope_exit resScopeExit([this, &res]() {
			this->env->DeleteLocalRef(res);
		});

		jstring str = static_cast<jstring>(res);

		auto chars = env->GetStringUTFChars(str, nullptr);
		utki::scope_exit charsScopeExit([this, &chars, &str]() {
			this->env->ReleaseStringUTFChars(str, chars);
		});

		return std::string(chars);
	}
};
} // namespace
