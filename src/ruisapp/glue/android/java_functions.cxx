#include "java_functions.hxx"

#include <utki/debug.hpp>

#include "globals.hxx"

java_functions_wrapper::java_functions_wrapper()
{
	auto a = globals_wrapper::native_activity;

	this->env = a->env;
	this->obj = a->clazz;
	this->clazz = this->env->GetObjectClass(this->obj);
	utki::assert(this->clazz, SL);

	this->resolve_key_unicode_method = this->env->GetMethodID(
		this->clazz,
		"resolveKeyUnicode",
		"(III)I" // function signature: three int arguemnts, returns int
	);
	utki::assert(this->resolve_key_unicode_method, SL);

	this->get_dots_per_inch_method = this->env->GetMethodID(
		this->clazz, //
		"getDotsPerInch",
		"()F" // function signature: no arguments, returns float
	);
	this->get_screen_resolution_method = this->env->GetMethodID(
		this->clazz,
		"getScreenResolution",
		"()[I" // function signature: no arguments, returns array of ints
	);

	this->list_dir_contents_method = this->env->GetMethodID(
		this->clazz, //
		"listDirContents",
		"(Ljava/lang/String;)[Ljava/lang/String;"
	);
	utki::assert(this->list_dir_contents_method, SL);

	this->show_virtual_keyboard_method = this->env->GetMethodID(
		this->clazz,
		"showVirtualKeyboard",
		"()V" // function signature: no arguments, returns void
	);
	utki::assert(this->show_virtual_keyboard_method, SL);

	this->hide_virtual_keyboard_method = this->env->GetMethodID(
		this->clazz,
		"hideVirtualKeyboard",
		"()V" // function signature: no arguments, returns void
	);
	utki::assert(this->hide_virtual_keyboard_method, SL);

	this->get_storage_dir_method = this->env->GetMethodID(
		this->clazz,
		"getStorageDir",
		"()Ljava/lang/String;" // function signature: no arguments, returns array of strings
	);
	utki::assert(this->get_storage_dir_method, SL);
}

char32_t java_functions_wrapper::resolve_key_unicode(
	int32_t dev_id, //
	int32_t meta_state,
	int32_t key_code
)
{
	return char32_t(
		this->env
			->CallIntMethod(this->obj, this->resolve_key_unicode_method, jint(dev_id), jint(meta_state), jint(key_code))
	);
}

float java_functions_wrapper::get_dots_per_inch()
{
	return float(this->env->CallFloatMethod(
		this->obj, //
		this->get_dots_per_inch_method
	));
}

r4::vector2<unsigned> java_functions_wrapper::get_screen_resolution()
{
	jobject res = this->env->CallObjectMethod(
		this->obj, //
		this->get_screen_resolution_method
	);

	utki::scope_exit scopeExit([this, res]() {
		this->env->DeleteLocalRef(res);
	});

	r4::vector2<unsigned> ret{0, 0};

	if (res == nullptr) {
		return ret;
	}

	jintArray arr = static_cast<jintArray>(res);

	jsize arr_count = env->GetArrayLength(arr);

	size_t count = std::max(0, arr_count); // clamp bottom

	jboolean is_copy;
	jint* elements = this->env->GetIntArrayElements(arr, &is_copy);

	utki::scope_exit elemnts_scope_exit([&]() {
		this->env->ReleaseIntArrayElements(arr, elements, 0);
	});

	for (jsize i = 0; i != std::min(count, ret.size()); ++i) {
		ret[i] = elements[i];
	}

	return ret;
}

void java_functions_wrapper::hide_virtual_keyboard()
{
	this->env->CallVoidMethod(
		this->obj, //
		this->hide_virtual_keyboard_method
	);
}

void java_functions_wrapper::show_virtual_keyboard()
{
	this->env->CallVoidMethod(
		this->obj, //
		this->show_virtual_keyboard_method
	);
}

std::vector<std::string> java_functions_wrapper::list_dir_contents(const std::string& path)
{
	jstring p = this->env->NewStringUTF(path.c_str());
	jobject res = this->env->CallObjectMethod(
		this->obj, //
		this->list_dir_contents_method,
		p
	);
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

std::string java_functions_wrapper::get_storage_dir()
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
