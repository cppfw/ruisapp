/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2025  Ivan Gagis <igagis@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* ================ LICENSE END ================ */

#include <cerrno>
#include <csignal>
#include <ctime>

#include <EGL/egl.h>
#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/window.h>
#include <nitki/queue.hpp>
#include <ruis/render/opengles/context.hpp>
#include <sys/eventfd.h>
#include <utki/destructable.hpp>
#include <utki/unicode.hpp>

#include "android_globals.hxx"

using namespace ruisapp;

namesapce
{
	ruisapp::application& get_app(ANativeActivity * activity)
	{
		// TODO:
		ASSERT(activity)
		ASSERT(activity->instance)
		return *static_cast<ruisapp::application*>(activity->instance);
	}

	ANativeWindow* android_window = nullptr;

	class java_functions_wrapper : public utki::destructable
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
		java_functions_wrapper(ANativeActivity* a)
		{
			this->env = a->env;
			this->obj = a->clazz;
			this->clazz = this->env->GetObjectClass(this->obj);
			ASSERT(this->clazz)

			this->resolve_key_unicode_method = this->env->GetMethodID(this->clazz, "resolveKeyUnicode", "(III)I");
			ASSERT(this->resolve_key_unicode_method)

			this->get_dots_per_inch_method = this->env->GetMethodID(this->clazz, "getDotsPerInch", "()F");

			this->list_dir_contents_method =
				this->env->GetMethodID(this->clazz, "listDirContents", "(Ljava/lang/String;)[Ljava/lang/String;");
			ASSERT(this->list_dir_contents_method)

			this->show_virtual_keyboard_method = this->env->GetMethodID(this->clazz, "showVirtualKeyboard", "()V");
			ASSERT(this->show_virtual_keyboard_method)

			this->hide_virtual_keyboard_method = this->env->GetMethodID(this->clazz, "hideVirtualKeyboard", "()V");
			ASSERT(this->hide_virtual_keyboard_method)

			this->get_storage_dir_method = this->env->GetMethodID(this->clazz, "getStorageDir", "()Ljava/lang/String;");
			ASSERT(this->get_storage_dir_method)
		}

		~java_functions_wrapper() noexcept {}

		char32_t resolve_key_unicode(int32_t devId, int32_t metaState, int32_t keyCode)
		{
			return char32_t(this->env->CallIntMethod(
				this->obj,
				this->resolve_key_unicode_method,
				jint(devId),
				jint(metaState),
				jint(keyCode)
			));
		}

		float get_dots_per_inch()
		{
			return float(this->env->CallFloatMethod(this->obj, this->get_dots_per_inch_method));
		}

		void hide_virtual_keyboard()
		{
			this->env->CallVoidMethod(this->obj, this->hide_virtual_keyboard_method);
		}

		void show_virtual_keyboard()
		{
			this->env->CallVoidMethod(this->obj, this->show_virtual_keyboard_method);
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

	std::unique_ptr<java_functions_wrapper> java_functions;

	struct window_wrapper : public utki::destructable {
		EGLDisplay display;
		EGLSurface surface = EGL_NO_SURFACE;
		EGLContext context = EGL_NO_CONTEXT;

		EGLint format;
		EGLConfig config;

		nitki::queue ui_queue;

		window_wrapper(const window_parameters& wp)
		{
			this->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
			if (this->display == EGL_NO_DISPLAY) {
				throw std::runtime_error("eglGetDisplay(): failed, no matching display connection found");
			}

			utki::scope_exit eglDisplayScopeExit([this]() {
				eglTerminate(this->display);
			});

			if (eglInitialize(this->display, 0, 0) == EGL_FALSE) {
				throw std::runtime_error("eglInitialize() failed");
			}

			auto graphics_api_version = [&ver = wp.graphics_api_version]() {
				if (ver.to_uint32_t() == 0) {
					// default OpenGL ES version is 2.0
					return utki::version_duplet{
						.major = 2, //
						.minor = 0
					};
				}
				return ver;
			}();

			// Specify the attributes of the desired configuration.
			// We need an EGLConfig with at least 8 bits per color
			// component compatible with on-screen windows.
			const std::array<EGLint, 15> attribs = {
				EGL_SURFACE_TYPE,
				EGL_WINDOW_BIT,
				EGL_RENDERABLE_TYPE,
				// We cannot set bits for all OpenGL ES versions because on platforms which do not
				// support later versions the matching config will not be found by eglChooseConfig().
				// So, set bits according to requested OpenGL ES version.
				[&ver = wp.graphics_api_version]() {
					EGLint ret = EGL_OPENGL_ES2_BIT; // OpenGL ES 2 is the minimum
					if (ver.major >= 3) {
						ret |= EGL_OPENGL_ES3_BIT;
					}
					return ret;
				}(),
				EGL_BLUE_SIZE,
				8,
				EGL_GREEN_SIZE,
				8,
				EGL_RED_SIZE,
				8,
				EGL_DEPTH_SIZE,
				wp.buffers.get(ruisapp::buffer::depth) ? int(utki::byte_bits * sizeof(uint16_t)) : 0,
				EGL_STENCIL_SIZE,
				wp.buffers.get(ruisapp::buffer::stencil) ? utki::byte_bits : 0,
				EGL_NONE
			};

			// Here, the application chooses the configuration it desires. In this
			// sample, we have a very simplified selection process, where we pick
			// the first EGLConfig that matches our criteria
			EGLint numConfigs;
			eglChooseConfig(this->display, attribs.data(), &this->config, 1, &numConfigs);
			if (numConfigs <= 0) {
				throw std::runtime_error("eglChooseConfig() failed, no matching config found");
			}

			// EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
			// guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
			// As soon as we picked a EGLConfig, we can safely reconfigure the
			// ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
			if (eglGetConfigAttrib(this->display, this->config, EGL_NATIVE_VISUAL_ID, &this->format) == EGL_FALSE) {
				throw std::runtime_error("eglGetConfigAttrib() failed");
			}

			std::array<EGLint, 5> context_attrs = {
				EGL_CONTEXT_MAJOR_VERSION,
				graphics_api_version.major,
				EGL_CONTEXT_MINOR_VERSION,
				graphics_api_version.minor,
				EGL_NONE
			};

			this->context = eglCreateContext(this->display, this->config, NULL, context_attrs.data());
			if (this->context == EGL_NO_CONTEXT) {
				throw std::runtime_error("eglCreateContext() failed");
			}

			utki::scope_exit eglContextScopeExit([this]() {
				eglDestroyContext(this->display, this->context);
			});

			this->create_surface();

			eglContextScopeExit.release();
			eglDisplayScopeExit.release();
		}

		void destroy_surface() noexcept
		{
			// according to
			// https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglMakeCurrent.xhtml
			// it is ok to destroy surface while EGL context is current, so here we do
			// not unbind the EGL context
			if (this->surface != EGL_NO_SURFACE) {
				eglDestroySurface(this->display, this->surface);
				this->surface = EGL_NO_SURFACE;
			}
		}

		void create_surface()
		{
			ASSERT(this->surface == EGL_NO_SURFACE)

			ASSERT(android_window)
			ANativeWindow_setBuffersGeometry(android_window, 0, 0, this->format);

			this->surface = eglCreateWindowSurface(this->display, this->config, android_window, NULL);
			if (this->surface == EGL_NO_SURFACE) {
				throw std::runtime_error("eglCreateWindowSurface() failed");
			}

			utki::scope_exit surface_scope_exit([this]() {
				this->destroy_surface();
			});

			// bind EGL context to the new surface
			ASSERT(this->context != EGL_NO_CONTEXT)
			eglMakeCurrent(this->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
						   EGL_NO_CONTEXT); // unbind EGL context
			if (eglMakeCurrent(this->display, this->surface, this->surface, this->context) == EGL_FALSE) {
				throw std::runtime_error("eglMakeCurrent() failed");
			}

			surface_scope_exit.release();
		}

		r4::vector2<unsigned> get_window_size()
		{
			if (this->surface == EGL_NO_SURFACE) {
				return {0, 0};
			}

			EGLint width, height;
			eglQuerySurface(this->display, this->surface, EGL_WIDTH, &width);
			eglQuerySurface(this->display, this->surface, EGL_HEIGHT, &height);
			return r4::vector2<unsigned>(width, height);
		}

		void swap_buffers()
		{
			if (this->surface == EGL_NO_SURFACE) {
				return;
			}

			eglSwapBuffers(this->display, this->surface);
		}

		void render(ruisapp::application& app)
		{
			if (this->surface == EGL_NO_SURFACE) {
				return;
			}

			ruisapp::render(app);
		}

		~window_wrapper() noexcept
		{
			eglMakeCurrent(this->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
						   EGL_NO_CONTEXT); // unbind EGL context
			eglDestroyContext(this->display, this->context);
			this->destroy_surface();
			eglTerminate(this->display);
		}
	};

	window_wrapper& get_impl(const std::unique_ptr<utki::destructable>& pimpl)
	{
		ASSERT(dynamic_cast<window_wrapper*>(pimpl.get()))
		return static_cast<window_wrapper&>(*pimpl);
	}

	window_wrapper& get_impl(application & app)
	{
		return get_impl(get_window_pimpl(app));
	}

	class asset_file : public papki::file
	{
		AAssetManager* manager;

		mutable AAsset* handle = nullptr;

	public:
		asset_file(
			AAssetManager* manager,
			// TODO: naming convention
			std::string_view pathName = std::string_view()
		) :
			manager(manager),
			papki::file(pathName)
		{
			ASSERT(this->manager)
		}

		virtual void open_internal(papki::mode mode) override
		{
			switch (mode) {
				case papki::mode::write:
				case papki::mode::create:
					throw std::invalid_argument(
						"'write' and 'create' open modes are not "
						"supported by Android assets"
					);
				case papki::mode::read:
					break;
				default:
					throw std::invalid_argument("unknown mode");
			}
			this->handle = AAssetManager_open(
				this->manager,
				this->path().c_str(),
				AASSET_MODE_UNKNOWN
			); // don't know what this MODE means at all
			if (!this->handle) {
				std::stringstream ss;
				ss << "AAssetManager_open(" << this->path() << ") failed";
				throw std::runtime_error(ss.str());
			}
		}

		virtual void close_internal() const noexcept override
		{
			ASSERT(this->handle)
			AAsset_close(this->handle);
			this->handle = 0;
		}

		virtual size_t read_internal(utki::span<std::uint8_t> buf) const override
		{
			ASSERT(this->handle)
			int numBytesRead = AAsset_read(this->handle, buf.data(), buf.size());
			if (numBytesRead < 0) { // something happened
				throw std::runtime_error("AAsset_read() error");
			}
			ASSERT(numBytesRead >= 0)
			return size_t(numBytesRead);
		}

		virtual size_t write_internal(utki::span<const std::uint8_t> buf) override
		{
			ASSERT(this->handle)
			throw std::runtime_error("write() is not supported by Android assets");
		}

		virtual size_t seek_forward_internal(size_t numBytesToSeek) const override
		{
			return this->seek(numBytesToSeek, true);
		}

		virtual size_t seek_backward_internal(size_t numBytesToSeek) const override
		{
			return this->seek(numBytesToSeek, false);
		}

		virtual void rewind_internal() const override
		{
			if (!this->is_open()) {
				throw std::logic_error("file is not opened, cannot rewind");
			}

			ASSERT(this->handle)
			if (AAsset_seek(this->handle, 0, SEEK_SET) < 0) {
				throw std::runtime_error("AAsset_seek() failed");
			}
		}

		virtual bool exists() const override
		{
			if (this->is_open()) { // file is opened => it exists
				return true;
			}

			if (this->path().size() == 0) {
				return false;
			}

			if (this->is_dir()) {
				// try opening the directory to check its existence
				AAssetDir* pdir = AAssetManager_openDir(this->manager, this->path().c_str());

				if (!pdir) {
					return false;
				} else {
					AAssetDir_close(pdir);
					return true;
				}
			} else {
				return this->file::exists();
			}
		}

		virtual std::vector<std::string> list_dir(size_t maxEntries = 0) const override
		{
			if (!this->is_dir()) {
				throw std::logic_error("asset_file::list_dir(): this is not a directory");
			}

			// Trim away trailing '/', as Android does not work with it.
			auto p = this->path().substr(0, this->path().size() - 1);

			ASSERT(java_functions)
			return java_functions->list_dir_contents(p);
		}

		std::unique_ptr<papki::file> spawn() override
		{
			return std::make_unique<asset_file>(this->manager);
		}

		~asset_file() noexcept {}

		size_t seek(size_t numBytesToSeek, bool seekForward) const
		{
			if (!this->is_open()) {
				throw std::logic_error("file is not opened, cannot seek");
			}

			ASSERT(this->handle)

			// NOTE: AAsset_seek() accepts 'off_t' as offset argument which is signed
			// and can be
			//       less than size_t value passed as argument to this function.
			//       Therefore, do several seek operations with smaller offset if
			//       necessary.

			off_t assetSize = AAsset_getLength(this->handle);
			ASSERT(assetSize >= 0)

			using std::min;
			if (seekForward) {
				ASSERT(size_t(assetSize) >= this->cur_pos())
				numBytesToSeek = min(numBytesToSeek, size_t(assetSize) - this->cur_pos()); // clamp top
			} else {
				numBytesToSeek = min(numBytesToSeek, this->cur_pos()); // clamp top
			}

			typedef off_t T_FSeekOffset;
			const size_t DMax = ((size_t(T_FSeekOffset(-1))) >> 1);
			ASSERT((size_t(1) << ((sizeof(T_FSeekOffset) * 8) - 1)) - 1 == DMax)
			static_assert(size_t(-(-T_FSeekOffset(DMax))) == DMax, "size mismatch");

			for (size_t numBytesLeft = numBytesToSeek; numBytesLeft != 0;) {
				ASSERT(numBytesLeft <= numBytesToSeek)

				T_FSeekOffset offset;
				if (numBytesLeft > DMax) {
					offset = T_FSeekOffset(DMax);
				} else {
					offset = T_FSeekOffset(numBytesLeft);
				}

				ASSERT(offset > 0)

				if (AAsset_seek(this->handle, seekForward ? offset : (-offset), SEEK_CUR) < 0) {
					throw std::runtime_error("AAsset_seek() failed");
				}

				ASSERT(size_t(offset) < size_t(-1))
				ASSERT(numBytesLeft >= size_t(offset))

				numBytesLeft -= size_t(offset);
			}
			return numBytesToSeek;
		}
	};

	ruis::vector2 cur_window_dims(0, 0);

	AInputQueue* input_queue = nullptr;

	// array of current pointer positions, needed to detect which pointers have
	// actually moved.
	std::array<ruis::vector2, 10> pointers;

	inline ruis::vector2 android_win_coords_to_ruis_win_rect_coords(const ruis::vector2& winDim, const ruis::vector2& p)
	{
		ruis::vector2 ret(p.x(), p.y() - (cur_window_dims.y() - winDim.y()));
		//	utki::log_debug([&](auto&o){o << "android_win_coords_to_ruis_win_rect_coords(): ret
		//= " << ret << std::endl;});
		using std::round;
		return round(ret);
	}

	struct android_configuration_wrapper {
		AConfiguration* android_configuration;

		android_configuration_wrapper()
		{
			this->android_configuration = AConfiguration_new();
		}

		~android_configuration_wrapper() noexcept
		{
			AConfiguration_delete(this->android_configuration);
		}
	};

	std::unique_ptr<android_configuration_wrapper> cur_config;

	class key_event_to_input_string_resolver : public ruis::gui::input_string_provider
	{
	public:
		int32_t kc; // key code
		int32_t ms; // meta state
		int32_t di; // device id

		std::u32string get() const
		{
			ASSERT(java_functions)
			//		utki::log_debug([&](auto&o){o << "key_event_to_unicode_resolver::Resolve():
			// this->kc = " << this->kc << std::endl;});
			char32_t res = java_functions->resolve_key_unicode(this->di, this->ms, this->kc);

			// 0 means that key did not produce any unicode character
			if (res == 0) {
				utki::log_debug([](auto& o) {
					o << "key did not produce any unicode character, returning empty string" << std::endl;
				});
				return std::u32string();
			}

			return std::u32string(&res, 1);
		}
	} key_input_string_resolver;

	//================
	// for updatable
	//================
	class event_fd_wrapper
	{
		int event_fd;

	public:
		event_fd_wrapper()
		{
			this->event_fd = eventfd(0, EFD_NONBLOCK);
			if (this->event_fd < 0) {
				throw std::system_error(errno, std::generic_category(), "could not create eventFD (*nix)");
			}
		}

		~event_fd_wrapper() noexcept
		{
			close(this->event_fd);
		}

		int get_fd() noexcept
		{
			return this->event_fd;
		}

		void set()
		{
			if (eventfd_write(this->event_fd, 1) < 0) {
				ASSERT(false)
			}
		}

		void clear()
		{
			eventfd_t value;
			if (eventfd_read(this->event_fd, &value) < 0) {
				if (errno == EAGAIN) {
					return;
				}
				ASSERT(false)
			}
		}
	} fd_flag;

	class linux_timer
	{
		timer_t timer;

		// Handler for SIGALRM signal
		static void on_SIGALRM(int)
		{
			fd_flag.set();
		}

	public:
		linux_timer()
		{
			int res = timer_create(
				CLOCK_MONOTONIC,
				0, // means SIGALRM signal is emitted when timer expires
				&this->timer
			);
			if (res != 0) {
				throw std::runtime_error("timer_create() failed");
			}

			struct sigaction sa;
			sa.sa_handler = &linux_timer::on_SIGALRM;
			sa.sa_flags = SA_NODEFER;
			memset(&sa.sa_mask, 0, sizeof(sa.sa_mask));

			res = sigaction(SIGALRM, &sa, 0);
			ASSERT(res == 0)
		}

		~linux_timer() noexcept
		{
			// set default handler for SIGALRM
			struct sigaction sa;
			sa.sa_handler = SIG_DFL;
			sa.sa_flags = 0;
			memset(&sa.sa_mask, 0, sizeof(sa.sa_mask));

#ifdef DEBUG
			int res =
#endif
				sigaction(SIGALRM, &sa, 0);
			ASSERT(res == 0, [&](auto& o) {
				o << " res = " << res << " errno = " << errno;
			})

#ifdef DEBUG
			res =
#endif
				timer_delete(this->timer);
			ASSERT(res == 0, [&](auto& o) {
				o << " res = " << res << " errno = " << errno;
			})
		}

		// if timer is already armed, it will re-set the expiration time
		void arm(uint32_t dt)
		{
			itimerspec ts;
			ts.it_value.tv_sec = dt / 1000;
			ts.it_value.tv_nsec = (dt % 1000) * 1000000;
			ts.it_interval.tv_nsec = 0; // one shot timer
			ts.it_interval.tv_sec = 0; // one shot timer

#ifdef DEBUG
			int res =
#endif
				timer_settime(this->timer, 0, &ts, 0);
			ASSERT(res == 0, [&](auto& o) {
				o << " res = " << res << " errno = " << errno;
			})
		}

		// returns true if timer was disarmed
		// returns false if timer has fired before it was disarmed.
		// TODO: this function is not used anywhere, remove?
		bool disarm()
		{
			itimerspec oldts;
			itimerspec newts;
			newts.it_value.tv_nsec = 0;
			newts.it_value.tv_sec = 0;

			int res = timer_settime(this->timer, 0, &newts, &oldts);
			if (res != 0) {
				ASSERT(false, [&](auto& o) {
					o << "errno = " << errno << " res = " << res;
				})
			}

			if (oldts.it_value.tv_nsec != 0 || oldts.it_value.tv_sec != 0) {
				return true;
			}
			return false;
		}
	} timer;

	ruis::key get_key_from_key_event(AInputEvent & event) noexcept
	{
		size_t kc = size_t(AKeyEvent_getKeyCode(&event));
		ASSERT(kc < key_code_map.size())
		return key_code_map[kc];
	}

	struct input_string_provider : public ruis::gui::input_string_provider {
		std::u32string chars;

		std::u32string get() const override
		{
			return std::move(this->chars);
		}
	};

} // namespace

namespace {

JNIEXPORT void JNICALL
Java_io_github_cppfw_ruisapp_RuisappActivity_handleCharacterStringInput(JNIEnv* env, jclass clazz, jstring chars)
{
	utki::log_debug([](auto& o) {
		o << "handleCharacterStringInput(): invoked" << std::endl;
	});

	const char* utf8Chars = env->GetStringUTFChars(chars, 0);

	utki::scope_exit scopeExit([env, &chars, utf8Chars]() {
		env->ReleaseStringUTFChars(chars, utf8Chars);
	});

	if (utf8Chars == nullptr || *utf8Chars == 0) {
		utki::log_debug([](auto& o) {
			o << "handleCharacterStringInput(): empty string passed in" << std::endl;
		});
		return;
	}

	utki::log_debug([&](auto& o) {
		o << "handleCharacterStringInput(): utf8Chars = " << utf8Chars << std::endl;
	});

	std::vector<char32_t> utf32;

	for (utki::utf8_iterator i(utf8Chars); !i.is_end(); ++i) {
		utf32.push_back(i.character());
	}

	input_string_provider provider;
	provider.chars = std::u32string(utf32.data(), utf32.size());

	//    utki::log_debug([&](auto&o){o << "handleCharacterStringInput(): provider.chars = "
	//    << provider.chars << std::endl;});

	ruisapp::handle_character_input(ruisapp::inst(), provider, ruis::key::unknown);
}

} // namespace

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	utki::log_debug([](auto& o) {
		o << "JNI_OnLoad(): invoked" << std::endl;
	});

	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		return -1;
	}

	static JNINativeMethod methods[] = {
		{"handleCharacterStringInput",
		 "(Ljava/lang/String;)V", (void*)&Java_io_github_cppfw_ruisapp_RuisappActivity_handleCharacterStringInput},
	};
	jclass clazz = env->FindClass("io/github/cppfw/ruisapp/RuisappActivity");
	ASSERT(clazz)
	if (env->RegisterNatives(clazz, methods, 1) < 0) {
		ASSERT(false)
	}

	return JNI_VERSION_1_6;
}

namespace {
ruisapp::application::directories get_application_directories(std::string_view app_name)
{
	ASSERT(java_functions)

	auto storage_dir = papki::as_dir(java_functions->get_storage_dir());

	ruisapp::application::directories dirs;

	dirs.cache = utki::cat(storage_dir, "cache/");
	dirs.config = utki::cat(storage_dir, "config/");
	dirs.state = utki::cat(storage_dir, "state/");

	return dirs;
}
} // namespace

ruisapp::application::application(
	std::string name, //
	const window_parameters& wp
) :
	name(name),
	window_pimpl(std::make_unique<window_wrapper>(wp)),
	gui(utki::make_shared<ruis::context>(
		utki::make_shared<ruis::style_provider>( //
			utki::make_shared<ruis::resource_loader>( //
				utki::make_shared<ruis::render::renderer>( //
					utki::make_shared<ruis::render::opengles::context>()
				)
			)
		),
		utki::make_shared<ruis::updater>(),
		ruis::context::parameters{
			.post_to_ui_thread_function =
				[this](std::function<void()> a) {
					get_impl(*this).ui_queue.push_back(std::move(a));
				},
			.set_mouse_cursor_function = [this](ruis::mouse_cursor) {},
			.units = ruis::units(
				[]() -> float {
					ASSERT(java_functions)

					return java_functions->get_dots_per_inch();
				}(),
				[this]() -> float {
					auto res = get_impl(*this).get_window_size();
					auto dim = (res.to<float>() / java_functions->get_dots_per_inch()) * float(utki::mm_per_inch);
					return application::get_pixels_per_pp(res, dim.to<unsigned>());
				}()
			)
		}
	)),
	directory(get_application_directories(this->name))
{
	auto win_size = get_impl(*this).get_window_size();
	this->update_window_rect(ruis::rect(ruis::vector2(0), win_size.to<ruis::real>()));
}

std::unique_ptr<papki::file> ruisapp::application::get_res_file(std::string_view path) const
{
	return std::make_unique<asset_file>(native_activity->assetManager, path);
}

void ruisapp::application::swap_frame_buffers()
{
	auto& ww = get_impl(*this);
	ww.swap_buffers();
}

void ruisapp::application::set_mouse_cursor_visible(bool visible)
{
	// do nothing
}

void ruisapp::application::set_fullscreen(bool enable)
{
	utki::assert(native_activity, SL);
	if (enable) {
		ANativeActivity_setWindowFlags(
			native_activity,
			AWINDOW_FLAG_FULLSCREEN, // add flags
			0 // remove flags
		);
	} else {
		ANativeActivity_setWindowFlags(
			native_activity,
			0, // add flags
			AWINDOW_FLAG_FULLSCREEN // remove flags
		);
	}
}

void ruisapp::application::quit() noexcept
{
	utki::assert(native_activity, SL);
	ANativeActivity_finish(native_activity);
}

void ruisapp::application::show_virtual_keyboard() noexcept
{
	// NOTE:
	// ANativeActivity_showSoftInput(native_activity,
	// ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED); did not work for some reason.

	ASSERT(java_functions)
	java_functions->show_virtual_keyboard();
}

void ruisapp::application::hide_virtual_keyboard() noexcept
{
	// NOTE:
	// ANativeActivity_hideSoftInput(native_activity,
	// ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS); did not work for some reason

	ASSERT(java_functions)
	java_functions->hide_virtual_keyboard();
}

namespace {
void handle_input_events()
{
	auto& app = ruisapp::inst();

	// read and handle input events
	AInputEvent* event;
	while (AInputQueue_getEvent(input_queue, &event) >= 0) {
		ASSERT(event)

		// utki::log_debug([&](auto&o){o << "New input event: type = " <<
		// AInputEvent_getType(event) << std::endl;});
		if (AInputQueue_preDispatchEvent(input_queue, event)) {
			continue;
		}

		int32_t eventType = AInputEvent_getType(event);
		int32_t eventAction = AMotionEvent_getAction(event);

		bool consume = false;

		switch (eventType) {
			case AINPUT_EVENT_TYPE_MOTION:
				switch (eventAction & AMOTION_EVENT_ACTION_MASK) {
					case AMOTION_EVENT_ACTION_POINTER_DOWN:
						// utki::log_debug([&](auto&o){o << "Pointer down" << std::endl;});
					case AMOTION_EVENT_ACTION_DOWN:
						{
							unsigned pointerIndex =
								((eventAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
								 AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
							unsigned pointerId = unsigned(AMotionEvent_getPointerId(event, pointerIndex));

							if (pointerId >= pointers.size()) {
								utki::log_debug([&](auto& o) {
									o << "Pointer ID is too big, only " << pointers.size()
									  << " pointers supported at maximum";
								});
								continue;
							}

							// utki::log_debug([&](auto&o){o << "Action down, ptr id = " << pointerId <<
							// std::endl;});

							ruis::vector2 p(
								AMotionEvent_getX(event, pointerIndex),
								AMotionEvent_getY(event, pointerIndex)
							);
							pointers[pointerId] = p;

							handle_mouse_button(
								app,
								true,
								android_win_coords_to_ruis_win_rect_coords(app.window_dims(), p),
								ruis::mouse_button::left,
								pointerId
							);
						}
						break;
					case AMOTION_EVENT_ACTION_POINTER_UP:
						// utki::log_debug([&](auto&o){o << "Pointer up" << std::endl;});
					case AMOTION_EVENT_ACTION_UP:
						{
							unsigned pointerIndex =
								((eventAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
								 AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
							unsigned pointerId = unsigned(AMotionEvent_getPointerId(event, pointerIndex));

							if (pointerId >= pointers.size()) {
								utki::log_debug([&](auto& o) {
									o << "Pointer ID is too big, only " << pointers.size()
									  << " pointers supported at maximum";
								});
								continue;
							}

							// utki::log_debug([&](auto&o){o << "Action up, ptr id = " << pointerId <<
							// std::endl;});

							ruis::vector2 p(
								AMotionEvent_getX(event, pointerIndex),
								AMotionEvent_getY(event, pointerIndex)
							);
							pointers[pointerId] = p;

							handle_mouse_button(
								app,
								false,
								android_win_coords_to_ruis_win_rect_coords(app.window_dims(), p),
								ruis::mouse_button::left,
								pointerId
							);
						}
						break;
					case AMOTION_EVENT_ACTION_MOVE:
						{
							size_t numPointers = AMotionEvent_getPointerCount(event);
							ASSERT(numPointers >= 1)
							for (size_t pointerNum = 0; pointerNum < numPointers; ++pointerNum) {
								unsigned pointerId = unsigned(AMotionEvent_getPointerId(event, pointerNum));
								if (pointerId >= pointers.size()) {
									utki::log_debug([&](auto& o) {
										o << "Pointer ID is too big, only " << pointers.size()
										  << " pointers supported at maximum";
									});
									continue;
								}

								// notify root container only if there was actual movement
								ruis::vector2 p(
									AMotionEvent_getX(event, pointerNum),
									AMotionEvent_getY(event, pointerNum)
								);
								if (pointers[pointerId] == p) {
									// pointer position did not change
									continue;
								}

								// utki::log_debug([&](auto&o){o << "Action move, ptr id = " << pointerId <<
								// std::endl;});

								pointers[pointerId] = p;

								handle_mouse_move(
									app,
									android_win_coords_to_ruis_win_rect_coords(app.window_dims(), p),
									pointerId
								);
							}
						}
						break;
					default:
						utki::log_debug([&](auto& o) {
							o << "unknown eventAction = " << eventAction << std::endl;
						});
						break;
				}
				consume = true;
				break;
			case AINPUT_EVENT_TYPE_KEY:
				{
					// utki::log_debug([&](auto&o){o << "AINPUT_EVENT_TYPE_KEY" << std::endl;});

					ASSERT(event)
					ruis::key key = get_key_from_key_event(*event);

					key_input_string_resolver.kc = AKeyEvent_getKeyCode(event);
					key_input_string_resolver.ms = AKeyEvent_getMetaState(event);
					key_input_string_resolver.di = AInputEvent_getDeviceId(event);

					// utki::log_debug([&](auto&o){o << "AINPUT_EVENT_TYPE_KEY:
					// key_input_string_resolver.kc = " << key_input_string_resolver.kc <<
					// std::endl;});

					switch (eventAction) {
						case AKEY_EVENT_ACTION_DOWN:
							// utki::log_debug([&](auto&o){o << "AKEY_EVENT_ACTION_DOWN, count = " <<
							// AKeyEvent_getRepeatCount(event) << std::endl;});

							// detect auto-repeated key events
							if (AKeyEvent_getRepeatCount(event) == 0) {
								handle_key_event(app, true, key);
							}
							handle_character_input(app, key_input_string_resolver, key);
							break;
						case AKEY_EVENT_ACTION_UP:
							// utki::log_debug([&](auto&o){o << "AKEY_EVENT_ACTION_UP" << std::endl;});
							handle_key_event(app, false, key);
							break;
						case AKEY_EVENT_ACTION_MULTIPLE:
							// utki::log_debug([&](auto&o){o << "AKEY_EVENT_ACTION_MULTIPLE"
							// 		<< " count = " << AKeyEvent_getRepeatCount(event)
							// 		<< " keyCode = " << AKeyEvent_getKeyCode(event)
							// 		<< std::endl;});

							// ignore, it is handled on Java side

							break;
						default:
							utki::log_debug([&](auto& o) {
								o << "unknown AINPUT_EVENT_TYPE_KEY eventAction: " << eventAction << std::endl;
							});
							break;
					}
				}
				break;
			default:
				break;
		}

		AInputQueue_finishEvent(input_queue, event, consume);
	}

	get_impl(app).render(app);

	fd_flag.set();
}
} // namespace

namespace {
void on_destroy(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_destroy(): invoked" << std::endl;
	});

	// TODO: move looper related stuff to window_wrapper?
	ALooper* looper = ALooper_prepare(0);
	ASSERT(looper)

	// remove UI message queue descriptor from looper
	ALooper_removeFd(looper, get_impl(application::inst()).ui_queue.get_handle());

	// remove fd_flag from looper
	ALooper_removeFd(looper, fd_flag.get_fd());

	delete static_cast<ruisapp::application*>(activity->instance);
	activity->instance = nullptr;

	java_functions.reset();
}

void on_start(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_start(): invoked" << std::endl;
	});
}

void on_resume(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_resume(): invoked" << std::endl;
	});
}

void* on_save_instance_state(ANativeActivity* activity, size_t* outSize)
{
	utki::log_debug([](auto& o) {
		o << "on_save_instance_state(): invoked" << std::endl;
	});

	// Do nothing, we don't use this mechanism of saving state.

	return nullptr;
}

void on_pause(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_pause(): invoked" << std::endl;
	});
}

void on_stop(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_stop(): invoked" << std::endl;
	});
}

void on_configuration_changed(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_configuration_changed(): invoked" << std::endl;
	});

	// find out what exactly has changed in the configuration
	int32_t diff;
	{
		auto config = std::make_unique<android_configuration_wrapper>();
		AConfiguration_fromAssetManager(config->android_configuration, native_activity->assetManager);

		diff = AConfiguration_diff(cur_config->android_configuration, config->android_configuration);

		// store new configuration
		cur_config = std::move(config);
	}

	// if orientation has changed
	if (diff & ACONFIGURATION_ORIENTATION) {
		int32_t orientation = AConfiguration_getOrientation(cur_config->android_configuration);
		switch (orientation) {
			case ACONFIGURATION_ORIENTATION_LAND:
			case ACONFIGURATION_ORIENTATION_PORT:
				using std::swap;
				swap(cur_window_dims.x(), cur_window_dims.y());
				break;
			case ACONFIGURATION_ORIENTATION_SQUARE:
				// do nothing
				break;
			case ACONFIGURATION_ORIENTATION_ANY:
				ASSERT(false)
			default:
				ASSERT(false)
				break;
		}
	}
}

void on_low_memory(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_low_memory(): invoked" << std::endl;
	});
}

void on_window_focus_changed(ANativeActivity* activity, int hasFocus)
{
	utki::log_debug([](auto& o) {
		o << "on_window_focus_changed(): invoked" << std::endl;
	});
}

int on_update_timer_expired(int fd, int events, void* data)
{
	//	utki::log_debug([&](auto&o){o << "on_update_timer_expired(): invoked" <<
	// std::endl;});

	auto& app = application::inst();

	uint32_t dt = app.gui.update();
	if (dt == 0) {
		// do not arm the timer and do not clear the flag
	} else {
		fd_flag.clear();
		timer.arm(dt);
	}

	// after updating need to re-render everything
	get_impl(app).render(app);

	//	utki::log_debug([&](auto&o){o << "on_update_timer_expired(): armed timer for " << dt
	//<< std::endl;});

	return 1; // 1 means do not remove descriptor from looper
}

int on_queue_has_messages(int fd, int events, void* data)
{
	auto& ww = get_impl(application::inst());

	while (auto m = ww.ui_queue.pop_front()) {
		m();
	}

	return 1; // 1 means do not remove descriptor from looper
}

void on_native_window_created(
	ANativeActivity* activity, //
	ANativeWindow* window
)
{
	utki::log_debug([](auto& o) {
		o << "on_native_window_created(): invoked" << std::endl;
	});

	// save window in a static var, so it is accessible for OpenGL initializers
	// from ruis::application class
	android_window = window;

	cur_window_dims.x() = float(ANativeWindow_getWidth(window));
	cur_window_dims.y() = float(ANativeWindow_getHeight(window));

	// If we have no application instance yet, create it now.
	// Otherwise the window was re-created after moving the app from background to foreground
	// and we just need to create EGL surface for the new window.
	if (!activity->instance) {
		try {
			// use local auto-pointer for now because an exception can be thrown and
			// need to delete object then.
			auto cfg = std::make_unique<android_configuration_wrapper>();
			// retrieve current configuration
			AConfiguration_fromAssetManager(cfg->android_configuration, native_activity->assetManager);

			application* app = ruisapp::application_factory::make_application(0, nullptr).release();

			// android application should always have GUI
			utki::assert_always(app, SL);

			activity->instance = app;

			// save current configuration in global variable
			cur_config = std::move(cfg);

			ALooper* looper = ALooper_prepare(0);
			ASSERT(looper)

			// add timer descriptor to looper, this is needed for updatable to work
			if (ALooper_addFd(
					looper,
					fd_flag.get_fd(),
					ALOOPER_POLL_CALLBACK,
					ALOOPER_EVENT_INPUT,
					&on_update_timer_expired,
					0
				) == -1)
			{
				throw std::runtime_error("failed to add timer descriptor to looper");
			}

			// add UI message queue descriptor to looper
			if (ALooper_addFd(
					looper,
					get_impl(*app).ui_queue.get_handle(),
					ALOOPER_POLL_CALLBACK,
					ALOOPER_EVENT_INPUT,
					&on_queue_has_messages,
					0
				) == -1)
			{
				throw std::runtime_error("failed to add UI message queue descriptor to looper");
			}

			// Set the fd_flag to call the update() for the first time if there
			// were any updateables started during creating application
			// object.
			fd_flag.set();

		} catch (std::exception& e) {
			utki::log_debug([&](auto& o) {
				o << "std::exception uncaught while creating application instance: " << e.what() << std::endl;
			});
			throw;
		} catch (...) {
			utki::log_debug([](auto& o) {
				o << "unknown exception uncaught while creating application instance!" << std::endl;
			});
			throw;
		}
	} else {
		get_impl(get_app(activity)).create_surface();
	}
}

void on_native_window_resized(ANativeActivity* activity, ANativeWindow* window)
{
	utki::log_debug([](auto& o) {
		o << "on_native_window_resized(): invoked" << std::endl;
	});

	// save window dimensions
	cur_window_dims.x() = float(ANativeWindow_getWidth(window));
	cur_window_dims.y() = float(ANativeWindow_getHeight(window));

	utki::log_debug([&](auto& o) {
		o << "on_native_window_resized(): cur_window_dims = " << cur_window_dims << std::endl;
	});
}

void on_native_window_redraw_needed(ANativeActivity* activity, ANativeWindow* window)
{
	utki::log_debug([](auto& o) {
		o << "on_native_window_redraw_needed(): invoked" << std::endl;
	});

	auto& app = get_app(activity);

	get_impl(app).render(app);
}

// This function is called right before destroying Window object, according to
// documentation:
// https://developer.android.com/ndk/reference/struct/a-native-activity-callbacks#onnativewindowdestroyed
void on_native_window_destroyed(ANativeActivity* activity, ANativeWindow* window)
{
	utki::log_debug([](auto& o) {
		o << "on_native_window_destroyed(): invoked" << std::endl;
	});

	// destroy EGL drawing surface associated with the window.
	// the EGL context remains existing and should preserve all resources like
	// textures, vertex buffers, etc.
	get_impl(get_app(activity)).destroy_surface();

	// delete configuration object
	cur_config.reset();

	android_window = nullptr;
}

int on_input_events_ready_for_reading_from_queue(int fd, int events, void* data)
{
	//	utki::log_debug([](auto&o){o << "on_input_events_ready_for_reading_from_queue():
	// invoked" << std::endl;});

	ASSERT(input_queue) // if we get events we should have input queue

	// if window is not created yet, ignore events
	if (!ruisapp::application::is_created()) {
		ASSERT(false)
		AInputEvent* event;
		while (AInputQueue_getEvent(input_queue, &event) >= 0) {
			if (AInputQueue_preDispatchEvent(input_queue, event)) {
				continue;
			}

			AInputQueue_finishEvent(input_queue, event, false);
		}
		return 1;
	}

	ASSERT(ruisapp::application::is_created())

	handle_input_events();

	return 1; // we don't want to remove input queue descriptor from looper
}

// NOTE: this callback is called before on_native_window_created()
void on_input_queue_created(ANativeActivity* activity, AInputQueue* queue)
{
	utki::log_debug([](auto& o) {
		o << "on_input_queue_created(): invoked" << std::endl;
	});
	ASSERT(queue);
	ASSERT(!input_queue)
	input_queue = queue;

	// attach queue to looper
	AInputQueue_attachLooper(
		input_queue,
		ALooper_prepare(0), // get looper for current thread (main thread)
		0, // 'ident' is ignored since we are using callback
		&on_input_events_ready_for_reading_from_queue,
		nullptr
	);
}

void on_input_queue_destroyed(ANativeActivity* activity, AInputQueue* queue)
{
	utki::log_debug([](auto& o) {
		o << "on_input_queue_destroyed(): invoked" << std::endl;
	});
	ASSERT(queue)
	ASSERT(input_queue == queue)

	// detach queue from looper
	AInputQueue_detachLooper(queue);

	input_queue = nullptr;
}

// Called when indow dimensions have changed, for example due to on-screen keyboard has been shown.
void on_content_rect_changed(ANativeActivity* activity, const ARect* rect)
{
	utki::log_debug([&](auto& o) {
		o << "on_content_rect_changed(): invoked, left = " << rect->left << " right = " << rect->right
		  << " top = " << rect->top << " bottom = " << rect->bottom << std::endl;
	});
	utki::log_debug([&](auto& o) {
		o << "on_content_rect_changed(): cur_window_dims = " << cur_window_dims << std::endl;
	});

	// Sometimes Android calls on_content_rect_changed() even after native window
	// was destroyed, i.e. on_native_window_destroyed() was called and, thus,
	// application object was destroyed. So need to check if our application is
	// still alive.
	if (!activity->instance) {
		utki::log_debug([&](auto& o) {
			o << "on_content_rect_changed(): application is not alive, ignoring "
				 "content rect change."
			  << std::endl;
		});
		return;
	}

	auto& app = get_app(activity);

	update_window_rect(
		app,
		ruis::rect(
			float(rect->left),
			cur_window_dims.y() - float(rect->bottom),
			float(rect->right - rect->left),
			float(rect->bottom - rect->top)
		)
	);

	// redraw, since WindowRedrawNeeded not always comes
	get_impl(app).render(app);
}
} // namespace

void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	utki::log_debug([&](auto& o) {
		o << "ANativeActivity_onCreate(): invoked" << std::endl;
	});

	utki::assert(!android_globals_wrapper::native_activity, SL);
	android_globals_wrapper::native_activity = activity;

	activity->callbacks->onDestroy = &on_destroy;
	activity->callbacks->onStart = &on_start;
	activity->callbacks->onResume = &on_resume;
	activity->callbacks->onSaveInstanceState = &on_save_instance_state;
	activity->callbacks->onPause = &on_pause;
	activity->callbacks->onStop = &on_stop;
	activity->callbacks->onConfigurationChanged = &on_configuration_changed;
	activity->callbacks->onLowMemory = &on_low_memory;
	activity->callbacks->onWindowFocusChanged = &on_window_focus_changed;
	activity->callbacks->onNativeWindowCreated = &on_native_window_created;
	activity->callbacks->onNativeWindowResized = &on_native_window_resized;
	activity->callbacks->onNativeWindowRedrawNeeded = &on_native_window_redraw_needed;
	activity->callbacks->onNativeWindowDestroyed = &on_native_window_destroyed;
	activity->callbacks->onInputQueueCreated = &on_input_queue_created;
	activity->callbacks->onInputQueueDestroyed = &on_input_queue_destroyed;
	activity->callbacks->onContentRectChanged = &on_content_rect_changed;

	activity->instance = new android_globals_wrapper();
	utki::assert(activity->instance, SL);

	// TODO: move to ruisapp_android_globals
	java_functions = std::make_unique<java_functions_wrapper>(native_activity);
}
