#include "TestingTypes.h"
#include <iostream>

int main()
{

	try {

		//TestingTMP();

		//TestingMisc();

		//TestingStrEncode();

		TestingSLRX();

		TestingReg();

		//TestingStrFormat();

		TestingEbnf();

		//TestingIntrusivePointer();
	}
	catch (std::exception const& E)
	{
		std::cout << E.what() << std::endl;
		return -1;
	}
	std::cout << "All Passed" << std::endl;
	return 0;
}
