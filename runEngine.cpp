/*
 * runEngine.cpp
 *
 *  Created on: 19.06.2018
 *      Author: jonas lehmann
 */

#include "VGE-V3M.hpp"

int main(int argc, char **argv)
{

	std::cout << threadCount << std::endl;

	if (threadCount <= 0)
		return EXIT_FAILURE;

	try
	{
		VGE_V3M app;
		app.play();

	} catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Success" << std::endl;
	return EXIT_SUCCESS;
}

