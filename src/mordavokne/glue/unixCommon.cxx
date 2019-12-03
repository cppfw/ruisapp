#include <dlfcn.h>

#include "../factory.hpp"

namespace{

std::unique_ptr<mordavokne::application> createAppUnix(int argc, const char** argv){
	void* libHandle = dlopen(nullptr, RTLD_NOW);
	if(!libHandle){
		throw morda::Exc("dlopen(): failed");
	}

	utki::ScopeExit scopeExit([libHandle](){
		dlclose(libHandle);
	});

	auto factory = reinterpret_cast<decltype(mordavokne::create_application)*>(
			dlsym(libHandle, "_ZN10mordavokne18create_applicationEiPPKc")
		);

	// TODO: deprecated, remove createApp() function search
	if(!factory){
		factory = reinterpret_cast<decltype(mordavokne::create_application)*>(
			dlsym(libHandle, "_ZN10mordavokne9createAppEiPPKc")
		);
	}

	if(!factory){
		throw morda::Exc("dlsym(): mordavokne::create_application() function not found!");
	}

	return factory(argc, argv);
}

std::string initializeStorageDir(const std::string& appName){
	auto homeDir = getenv("HOME");
	if(!homeDir){
		throw utki::Exc("failed to get user home directory. Is HOME environment variable set?");
	}

	std::string homeDirStr(homeDir);

	if(*homeDirStr.rend() != '/'){
		homeDirStr.append(1, '/');
	}

	homeDirStr.append(1, '.').append(appName).append(1, '/');

	papki::FSFile dir(homeDirStr);
	if(!dir.exists()){
		dir.makeDir();
	}

	return homeDirStr;
}

}