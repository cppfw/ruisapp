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

#include <utki/unicode.hpp>

#include "application.hxx"
#include "globals.hxx"
#include "key_code_map.hxx"

// include implementations
#include "android_configuration.cxx"
#include "application.cxx"
#include "globals.cxx"
#include "java_functions.cxx"
#include "linux_timer.cxx"
#include "window.cxx"

namespace {
class key_event_to_input_string_resolver : public ruis::gui::input_string_provider
{
public:
	int32_t kc; // key code
	int32_t ms; // meta state
	int32_t di; // device id

	std::u32string get() const
	{
		auto& glob = get_glob();

		//		utki::log_debug([&](auto&o){o << "key_event_to_unicode_resolver::Resolve():
		// this->kc = " << this->kc << std::endl;});
		char32_t res = glob.java_functions.resolve_key_unicode(
			this->di, //
			this->ms,
			this->kc
		);

		// 0 means that key did not produce any unicode character
		if (res == 0) {
			utki::log_debug([](auto& o) {
				o << "key did not produce any unicode character, returning empty string" << std::endl;
			});
			return std::u32string();
		}

		return std::u32string(&res, 1);
	}
};

ruis::key get_key_from_key_event(AInputEvent& event) noexcept
{
	size_t kc = size_t(AKeyEvent_getKeyCode(&event));
	utki::assert(kc < key_code_map.size(), SL);
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

JNIEXPORT void JNICALL Java_io_github_cppfw_ruisapp_RuisappActivity_handleCharacterStringInput(
	JNIEnv* env, //
	jclass clazz,
	jstring chars
)
{
	utki::log_debug([](auto& o) {
		o << "handleCharacterStringInput(): invoked" << std::endl;
	});

	const char* utf8Chars = env->GetStringUTFChars(chars, 0);

	utki::scope_exit utf8_chars_scope_exit([env, &chars, utf8Chars]() {
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

	auto& glue = get_glue();

	if (auto win = glue.get_window()) {
		win->gui.send_character_input(provider, ruis::key::unknown);
	}
}

} // namespace

// this has to be named exactly JNI_OnLoad()
jint JNI_OnLoad(
	JavaVM* vm, //
	void* reserved
)
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
	utki::assert(clazz, SL);
	if (env->RegisterNatives(clazz, methods, 1) < 0) {
		utki::assert(false, SL);
	}

	return JNI_VERSION_1_6;
}

namespace {
void handle_input_events()
{
	auto& glob = get_glob();
	utki::assert(glob.input_queue, SL);

	auto& glue = get_glue();

	key_event_to_input_string_resolver key_input_string_resolver;

	auto* win = glue.get_window();

	// read and handle input events
	for (AInputEvent* event = nullptr; AInputQueue_getEvent(glob.input_queue, &event) >= 0;) {
		utki::assert(event, SL);

		// utki::log_debug([&](auto&o){o << "New input event: type = " <<
		// AInputEvent_getType(event) << std::endl;});
		if (AInputQueue_preDispatchEvent(glob.input_queue, event)) {
			continue;
		}

		bool consume = false;

		utki::scope_exit finish_event_sope_exit([&]() {
			AInputQueue_finishEvent(
				glob.input_queue, //
				event,
				consume
			);
		});

		if (!win) {
			// we have no window, thus, do not handle any input events
			continue;
		}

		int32_t event_action = AMotionEvent_getAction(event);

		switch (AInputEvent_getType(event)) {
			case AINPUT_EVENT_TYPE_MOTION:
				switch (event_action & AMOTION_EVENT_ACTION_MASK) {
					case AMOTION_EVENT_ACTION_POINTER_DOWN:
						// utki::log_debug([&](auto&o){o << "Pointer down" << std::endl;});
					case AMOTION_EVENT_ACTION_DOWN:
						{
							unsigned pointer_index =
								((event_action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
								 AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
							unsigned pointer_id = unsigned(AMotionEvent_getPointerId(event, pointer_index));

							if (pointer_id >= glob.pointers.size()) {
								utki::log_debug([&](auto& o) {
									o << "Pointer ID is too big, only " << glob.pointers.size()
									  << " pointers supported at maximum";
								});
								continue;
							}

							utki::log_debug([&](auto& o) {
								o << "Action down, ptr id = " << pointer_id << std::endl;
							});

							ruis::vector2 p(
								AMotionEvent_getX(event, pointer_index),
								AMotionEvent_getY(event, pointer_index)
							);
							glob.pointers[pointer_id] = p;

							utki::assert(win, SL);

							win->gui.send_mouse_button(
								true, // is_down
								win->android_win_coords_to_ruisapp_win_rect_coords(p), // pos
								ruis::mouse_button::left,
								pointer_id
							);
						}
						break;
					case AMOTION_EVENT_ACTION_POINTER_UP:
						// utki::log_debug([&](auto&o){o << "Pointer up" << std::endl;});
					case AMOTION_EVENT_ACTION_UP:
						{
							unsigned pointer_index =
								((event_action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
								 AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
							unsigned pointer_id = unsigned(AMotionEvent_getPointerId(event, pointer_index));

							if (pointer_id >= glob.pointers.size()) {
								utki::log_debug([&](auto& o) {
									o << "Pointer ID is too big, only " << glob.pointers.size()
									  << " pointers supported at maximum";
								});
								continue;
							}

							utki::log_debug([&](auto& o) {
								o << "Action up, ptr id = " << pointer_id << std::endl;
							});

							ruis::vector2 p(
								AMotionEvent_getX(event, pointer_index),
								AMotionEvent_getY(event, pointer_index)
							);
							glob.pointers[pointer_id] = p;

							utki::assert(win, SL);

							win->gui.send_mouse_button(
								false, // is_down
								win->android_win_coords_to_ruisapp_win_rect_coords(p), // pos
								ruis::mouse_button::left,
								pointer_id
							);
						}
						break;
					case AMOTION_EVENT_ACTION_MOVE:
						{
							size_t num_pointers = AMotionEvent_getPointerCount(event);
							utki::assert(num_pointers >= 1, SL);
							for (size_t pointer_num = 0; pointer_num < num_pointers; ++pointer_num) {
								unsigned pointer_id = unsigned(AMotionEvent_getPointerId(event, pointer_num));
								if (pointer_id >= glob.pointers.size()) {
									utki::log_debug([&](auto& o) {
										o << "Pointer ID is too big, only " << glob.pointers.size()
										  << " pointers supported at maximum";
									});
									continue;
								}

								// notify root container only if there was actual movement
								ruis::vector2 p(
									AMotionEvent_getX(event, pointer_num),
									AMotionEvent_getY(event, pointer_num)
								);
								if (glob.pointers[pointer_id] == p) {
									// pointer position did not change
									continue;
								}

								// utki::log_debug([&](auto&o){o << "Action move, ptr id = " << pointer_id <<
								// std::endl;});

								glob.pointers[pointer_id] = p;

								utki::assert(win, SL);
								win->gui.send_mouse_move(
									win->android_win_coords_to_ruisapp_win_rect_coords(p), // pos
									pointer_id
								);
							}
						}
						break;
					default:
						utki::log_debug([&](auto& o) {
							o << "unknown event_action = " << event_action << std::endl;
						});
						break;
				}
				consume = true;
				break;
			case AINPUT_EVENT_TYPE_KEY:
				{
					// utki::log_debug([&](auto&o){o << "AINPUT_EVENT_TYPE_KEY" << std::endl;});

					utki::assert(event, SL);
					ruis::key key = get_key_from_key_event(*event);

					key_input_string_resolver.kc = AKeyEvent_getKeyCode(event);
					key_input_string_resolver.ms = AKeyEvent_getMetaState(event);
					key_input_string_resolver.di = AInputEvent_getDeviceId(event);

					// utki::log_debug([&](auto&o){o << "AINPUT_EVENT_TYPE_KEY:
					// key_input_string_resolver.kc = " << key_input_string_resolver.kc <<
					// std::endl;});

					switch (event_action) {
						case AKEY_EVENT_ACTION_DOWN:
							// utki::log_debug([&](auto&o){o << "AKEY_EVENT_ACTION_DOWN, count = " <<
							// AKeyEvent_getRepeatCount(event) << std::endl;});

							utki::assert(win, SL);

							// detect auto-repeated key events
							if (AKeyEvent_getRepeatCount(event) == 0) {
								win->gui.send_key(
									true, // is_down
									key
								);
							}

							win->gui.send_character_input(
								key_input_string_resolver, //
								key
							);
							break;
						case AKEY_EVENT_ACTION_UP:
							// utki::log_debug([&](auto&o){o << "AKEY_EVENT_ACTION_UP" << std::endl;});
							win->gui.send_key(
								false, // is_down
								key
							);
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
								o << "unknown AINPUT_EVENT_TYPE_KEY event_action: " << event_action << std::endl;
							});
							break;
					}
				}
				break;
			default:
				break;
		}
	}

	glue.render();

	glob.main_loop_event_fd.set();
}
} // namespace

namespace {
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

void* on_save_instance_state(
	ANativeActivity* activity, //
	size_t* out_size
)
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
	utki::assert(globals_wrapper::native_activity, SL);
	utki::assert(activity == globals_wrapper::native_activity, SL);

	utki::log_debug([](auto& o) {
		o << "on_configuration_changed(): invoked" << std::endl;
	});

	auto& glob = get_glob();

	// find out what exactly has changed in the configuration
	utki::assert(activity->assetManager, SL);
	auto config = utki::make_unique<android_configuration_wrapper>(*activity->assetManager);

	int32_t diff = glob.cur_android_configuration.get().diff(config);

	// store new configuration
	glob.cur_android_configuration = std::move(config);

	// if orientation has changed
	if (diff & ACONFIGURATION_ORIENTATION) {
		switch (glob.cur_android_configuration.get().get_orientation()) {
			case ACONFIGURATION_ORIENTATION_LAND:
			case ACONFIGURATION_ORIENTATION_PORT:
				using std::swap;
				swap(glob.cur_window_dims.x(), glob.cur_window_dims.y());
				break;
			case ACONFIGURATION_ORIENTATION_SQUARE:
				// do nothing
				break;
			case ACONFIGURATION_ORIENTATION_ANY:
				utki::assert(false, SL);
			default:
				utki::assert(false, SL);
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

void on_window_focus_changed(
	ANativeActivity* activity, //
	int has_focus
)
{
	utki::log_debug([](auto& o) {
		o << "on_window_focus_changed(): invoked" << std::endl;
	});
}

void on_native_window_created(
	ANativeActivity* activity, //
	ANativeWindow* window
)
{
	utki::assert(globals_wrapper::native_activity, SL);
	utki::assert(activity == globals_wrapper::native_activity, SL);

	utki::assert(window, SL);

	utki::log_debug([](auto& o) {
		o << "on_native_window_created(): invoked" << std::endl;
	});

	auto& glob = get_glob();

	glob.cur_window_dims.x() = ruis::real(ANativeWindow_getWidth(window));
	glob.cur_window_dims.y() = ruis::real(ANativeWindow_getHeight(window));

	auto& glue = get_glue();

	// The android window was just initially created or was re-created after moving the app
	// from background to foreground. In any case we just need to create EGL surface for the new android window.
	glue.create_window_surface(*window);
}

void on_native_window_resized(
	ANativeActivity* activity, //
	ANativeWindow* window
)
{
	utki::log_debug([](auto& o) {
		o << "on_native_window_resized(): invoked" << std::endl;
	});

	utki::assert(window, SL);

	auto& glob = get_glob();

	// save window dimensions
	glob.cur_window_dims.x() = ruis::real(ANativeWindow_getWidth(window));
	glob.cur_window_dims.y() = ruis::real(ANativeWindow_getHeight(window));

	utki::logcat_debug(
		"glob.cur_window_dims = ", //
		glob.cur_window_dims,
		'\n'
	);

	// expecting that on_content_rect_changed() will be called right after
	// and it will update the GL viewport

	utki::log_debug([&](auto& o) {
		o << "on_native_window_resized(): cur_window_dims = " << glob.cur_window_dims << std::endl;
	});
}

void on_native_window_redraw_needed(
	ANativeActivity* activity, //
	ANativeWindow* window
)
{
	utki::log_debug([](auto& o) {
		o << "on_native_window_redraw_needed(): invoked" << std::endl;
	});

	auto& glue = get_glue();
	glue.render();
}

// This function is called right before destroying Window object, according to
// documentation:
// https://developer.android.com/ndk/reference/struct/a-native-activity-callbacks#onnativewindowdestroyed
void on_native_window_destroyed(
	ANativeActivity* activity, //
	ANativeWindow* window
)
{
	utki::log_debug([](auto& o) {
		o << "on_native_window_destroyed(): invoked" << std::endl;
	});

	auto& glue = get_glue();

	// destroy EGL drawing surface associated with the window.
	// the EGL context remains existing and should preserve all resources like
	// textures, vertex buffers, etc.
	glue.destroy_window_surface();
}

int on_input_events_ready_for_reading_from_queue(
	int fd, //
	int events,
	void* data
)
{
	//	utki::log_debug([](auto&o){o << "on_input_events_ready_for_reading_from_queue():
	// invoked" << std::endl;});

	auto& glob = get_glob();

	// if we get events we should have input queue
	utki::assert(glob.input_queue, SL);

	// if window is not created yet, ignore events
	if (!ruisapp::application::is_created()) {
		// normally, should not get here
		utki::assert(false, SL);

		AInputEvent* event;
		while (AInputQueue_getEvent(
				   glob.input_queue, //
				   &event
			   ) >= 0)
		{
			if (AInputQueue_preDispatchEvent(
					glob.input_queue, //
					event
				))
			{
				continue;
			}

			AInputQueue_finishEvent(
				glob.input_queue, //
				event,
				false
			);
		}
		return 1; // we don't want to remove input queue descriptor from looper
	}

	utki::assert(ruisapp::application::is_created(), SL);

	handle_input_events();

	return 1; // we don't want to remove input queue descriptor from looper
}

// NOTE: this callback is called before on_native_window_created()
void on_input_queue_created(
	ANativeActivity* activity, //
	AInputQueue* queue
)
{
	utki::log_debug([](auto& o) {
		o << "on_input_queue_created(): invoked" << std::endl;
	});

	auto& glob = get_glob();

	utki::assert(queue, SL);
	utki::assert(!glob.input_queue, SL);
	glob.input_queue = queue;

	// attach queue to looper
	AInputQueue_attachLooper(
		glob.input_queue,
		// TODO: use glob.looper?
		ALooper_prepare(0), // get looper for current thread (main thread)
		0, // 'ident' is ignored since we are using callback
		&on_input_events_ready_for_reading_from_queue,
		nullptr
	);
}

void on_input_queue_destroyed(
	ANativeActivity* activity, //
	AInputQueue* queue
)
{
	utki::log_debug([](auto& o) {
		o << "on_input_queue_destroyed(): invoked" << std::endl;
	});

	auto& glob = get_glob();

	utki::assert(glob.input_queue, SL);
	utki::assert(glob.input_queue == queue, SL);

	// detach queue from looper
	AInputQueue_detachLooper(glob.input_queue);

	glob.input_queue = nullptr;
}

// Called when window dimensions have changed, for example due to on-screen keyboard has been shown.
void on_content_rect_changed(
	ANativeActivity* activity, //
	const ARect* rect
)
{
	auto& glob = get_glob();
	auto& glue = get_glue();

	utki::log_debug([&](auto& o) {
		o << "on_content_rect_changed(): invoked, left = " << rect->left << " right = " << rect->right
		  << " top = " << rect->top << " bottom = " << rect->bottom << std::endl;
	});
	utki::log_debug([&](auto& o) {
		o << "on_content_rect_changed(): cur_window_dims = " << glob.cur_window_dims << std::endl;
	});

	auto win = glue.get_window();

	// if we got on_content_rect_changed() then we should have a window
	utki::assert(win, SL);

	win->set_win_rect( //
		ruis::rect(
			float(rect->left), //
			float(rect->top),
			float(rect->right - rect->left),
			float(rect->bottom - rect->top)
		)
	);

	// TODO: request redraw instead of redrawing right here?
	// redraw, since WindowRedrawNeeded not always comes
	glue.render();
}

void on_destroy(ANativeActivity* activity)
{
	utki::log_debug([](auto& o) {
		o << "on_destroy(): invoked" << std::endl;
	});

	globals_wrapper::destroy();
}
} // namespace

void ANativeActivity_onCreate(
	ANativeActivity* activity, //
	void* saved_state,
	size_t saved_state_size
)
{
	utki::log_debug([&](auto& o) {
		o << "ANativeActivity_onCreate(): invoked" << std::endl;
	});

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

	globals_wrapper::create(activity);
}
