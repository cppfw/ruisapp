#include <ruisapp/application.hpp>

#include <iostream>

const ruisapp::application_factory app_fac([](auto executable, auto args){

	std::cout << "Hello ruisapp!" << std::endl;

	return nullptr;
});
