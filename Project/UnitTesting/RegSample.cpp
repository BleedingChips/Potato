#include "Potato/Public/PotatoReg.h"
#include "TestingTypes.h"
using namespace Potato::Reg;


void TestingReg()
{
	{
		std::u32string_view Str = UR"(475746)";
		Table Tab1(UR"([^123]+)");
		auto Result = ProcessMarch(Tab1.AsWrapper(), Str);
		if (!Result.has_value())
			throw UnpassedUnit{ "TestingReg : Bad Sample 1" };

		std::u32string_view Str2 = UR"(123475746)";
		auto Result2 = ProcessSearch(Tab1.AsWrapper(), Str2);
		if (!Result2.has_value() || Str2.substr(Result2->MainCapture.Begin(), Result2->MainCapture.Count()) != Str2.substr(3))
			throw UnpassedUnit{ "TestingReg : Bad Sample 2" };

		Table Tab2(UR"([123]{1,4})");
		std::u32string_view Str3 = UR"(645123475746)";
		auto Result3 = ProcessSearch(Tab2.AsWrapper(), Str3);
		if (!Result3.has_value() || Str3.substr(Result3->MainCapture.Begin(), Result3->MainCapture.Count()) != Str3.substr(3, 3))
			throw UnpassedUnit{ "TestingReg : Bad Sample 3" };
	}

	std::wcout << LR"(TestingReg Pass !)" << std::endl;
}