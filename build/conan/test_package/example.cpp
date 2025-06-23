#include <ruisapp/application.hpp>

const ruisapp::application_factory app_fac([](auto executable, auto args){

	std::cout << "Hello ruisapp!" << std::endl;

	return nullptr;
});
