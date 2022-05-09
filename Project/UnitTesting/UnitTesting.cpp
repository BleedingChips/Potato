#include "TestingTypes.h"



#include "Potato/Public/PotatoDLr.h"
#include "Potato/Public/PotatoReg.h"
#include "Potato/Public/PotatoStrEncode.h"
#include "Potato/Public/PotatoStrFormat.h"
#include "Potato/Public/PotatoEbnf.h"
#include <iostream>

using namespace Potato;

int main()
{

	try {

		TestingTMP();

		TestingMisc();

		TestingStrEncode();

		TestingDLr();

		TestingReg();

		TestingStrFormat();

		TestingEbnf();

		TestingIntrusivePointer();
	}
	catch (std::exception const& E)
	{
		std::cout << E.what() << std::endl;
		return -1;
	}
	std::cout << "All Passed" << std::endl;
	return 0;
}
