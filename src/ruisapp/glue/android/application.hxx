#pragma once

#include <utki/destructable.hpp>

#include "../../application.hpp"
#include "../../window.hpp"

namespace {
class app_window : public ruisapp::window
{
public:
};
} // namespace

namespace {
class application_glue : public utki::destructable
{
public:
};
} // namespace

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
