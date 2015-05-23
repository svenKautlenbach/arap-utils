#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ArapUtils.h"

namespace
{
}

int main (int argc, char* argv[])
{
	try
	{
		std::cout << "Starting tests..." << std::endl;

		std::cout << "SplittingStringBySpace()-";
		auto lines = arap::strings::Utilities::split("Jou mees tere.", " ");
		if (lines.size() == 3)
		{
			std::cout << "SUCCESS" << std::endl;
		}
		else
		{
			std::cout << "FAIL" << std::endl;
		}

		std::cout << "ConvertingEui64From_aaaa_0()-";
		auto eui64 = arap::network::Ipv6MacConvert::getEui64("aaaa::1");
		std::cout << "SUCCESS" << std::endl;
		
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "Received runtime error - " << e.what() << std::endl;

		return -4;
	}
	catch (const std::bad_alloc& e)
	{
		std::cerr << "Received bad allocation - " << e.what() << std::endl;

		return -5;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Received exception - " << e.what() << std::endl;

		return -6;
	}
	catch (...)
	{
		std::cerr << "Received unknown exception." << std::endl;

		return -7;
	}

	std::cout << "Test succesful." << std::endl;
	
	return 0;
}

