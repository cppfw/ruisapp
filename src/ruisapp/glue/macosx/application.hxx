#pragma once

#include <atomic>

#include <utki/destructable.hpp>

#import <Cocoa/Cocoa.h>

#include "../../application.hpp"

#include "display.hxx"

namespace{
class application_glue : public utki::destructable {
    // TODO: is needed?
    // utki::shared_ref<display_wrapper> display = utki::make_shared<display_wrapper>();

public:
    std::atomic_bool quit_flag = false;

    struct macos_application_wrapper{
        const NSApplication* application;

        macos_application_wrapper() :
            application([NSApplication sharedApplication])
        {
            if(!this->application){
                throw std::runtime_error("failed to create NSApplication instance");
            }

            // TODO: why is this needed?
            [this->application activateIgnoringOtherApps:YES];
        }

        macos_application_wrapper(const macos_application_wrapper&) = delete;
        macos_application_wrapper& operator=(const macos_application_wrapper&) = delete;

        macos_application_wrapper(macos_application_wrapper&&) = delete;
        macos_application_wrapper& operator=(macos_application_wrapper&&) = delete;

        ~macos_application_wrapper(){
            [this->application release];
        }
    } macos_application;

    // TODO:
};
}

namespace {
inline application_glue& get_glue(ruisapp::application& app)
{
	return static_cast<application_glue&>(app.pimpl.get());
}
} // namespace

namespace {
inline application_glue& get_glue()
{
	return get_glue(ruisapp::application::inst());
}
} // namespace
