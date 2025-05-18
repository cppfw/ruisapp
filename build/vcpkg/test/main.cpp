// this file is to test basic usage of the library,
// mainly test that linking to the library works, so just call any
// non-inline function of the library to test it.

#include <ruisapp/application.hpp>

namespace{
ruisapp::application_factory appfac(
    [](std::string_view executable, utki::span<std::string_view> args) -> std::unique_ptr<ruisapp::application> {
        std::cout << "Hello ruisapp!" << std::endl;
        return nullptr;
    }
);
}
