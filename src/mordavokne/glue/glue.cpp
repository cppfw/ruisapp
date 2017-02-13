#include <utki/config.hpp>

#include "../App.hpp"

#if M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_UNIX

#	include <dlfcn.h>

namespace{

std::unique_ptr<mordavokne::App> createAppUnix(int argc, const char** argv, const utki::Buf<std::uint8_t> savedState){
	void* libHandle = dlopen(nullptr, RTLD_NOW);
	if(!libHandle){
		throw morda::Exc("dlopen(): failed");
	}

	utki::ScopeExit scopeExit([libHandle](){
		dlclose(libHandle);
	});

	auto factory =
			reinterpret_cast<
					std::unique_ptr<mordavokne::App> (*)(int, const char**, const utki::Buf<std::uint8_t>)
				>(dlsym(libHandle, "_ZN10mordavokne9createAppEiPPKcN4utki3BufIhEE"));
	if(!factory){
		throw morda::Exc("dlsym(): createApp() function not found!");
	}

	return factory(argc, argv, savedState);
}

}
#endif


#include "friendAccessors.cppinc"


#if M_OS == M_OS_WINDOWS
#	include "windows/glue.cppinc"
#elif M_OS == M_OS_LINUX && M_OS_NAME == M_OS_NAME_ANDROID
#	include "android/glue.cppinc"
#elif M_OS == M_OS_LINUX
#	include "linux/glue.cppinc"
#endif
