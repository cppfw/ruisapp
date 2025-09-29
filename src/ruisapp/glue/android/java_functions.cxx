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

	this->resolve_key_unicode_method = this->env->GetMethodID(this->clazz, "resolveKeyUnicode", "(III)I");
	utki::assert(this->resolve_key_unicode_method, SL);

	this->get_dots_per_inch_method = this->env->GetMethodID(this->clazz, "getDotsPerInch", "()F");

	this->list_dir_contents_method = this->env->GetMethodID(
		this->clazz, //
		"listDirContents",
		"(Ljava/lang/String;)[Ljava/lang/String;"
	);
	utki::assert(this->list_dir_contents_method, SL);

	this->show_virtual_keyboard_method = this->env->GetMethodID(this->clazz, "showVirtualKeyboard", "()V");
	utki::assert(this->show_virtual_keyboard_method, SL);

	this->hide_virtual_keyboard_method = this->env->GetMethodID(this->clazz, "hideVirtualKeyboard", "()V");
	utki::assert(this->hide_virtual_keyboard_method, SL);

	this->get_storage_dir_method = this->env->GetMethodID(this->clazz, "getStorageDir", "()Ljava/lang/String;");
	utki::assert(this->get_storage_dir_method, SL);
}
