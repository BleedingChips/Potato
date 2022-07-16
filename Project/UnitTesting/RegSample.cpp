﻿#include "Potato/Public/PotatoReg.h"
#include "TestingTypes.h"
using namespace Potato::Reg;


void TestingReg()
{
	/*
	{
		std::u32string_view Str = UR"(123456abcdef)";
		std::u32string_view TStr = UR"(123456)";

		Table T(TStr);

		auto R = ProcessFrontMatch(T.AsWrapper(), Str);

		if (!R.has_value() || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != TStr)
			throw UnpassedUnit{ "TestingReg : Bad Sample 1" };
	}

	{
		std::u32string_view Str = UR"(2)";

		Table T(UR"([0-9])");

		auto R = ProcessMatch(T.AsWrapper(), Str);

		if (!R.has_value() || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != Str)
			throw UnpassedUnit{ "TestingReg : Bad Sample 2" };
	}

	{
		std::u32string_view Str = UR"(11111)";

		Table T(UR"(1*)");

		auto R = ProcessMatch(T.AsWrapper(), Str);

		if (!R.has_value() || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != Str)
			throw UnpassedUnit{ "TestingReg : Bad Sample 3" };
	}

	{
		std::u32string_view Str = UR"(11111)";

		Table T(UR"(1+)");

		auto R = ProcessMatch(T.AsWrapper(), Str);

		if (!R.has_value() || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != Str)
			throw UnpassedUnit{ "TestingReg : Bad Sample 4" };
	}

	

	{
		std::u32string_view Str = UR"(11112222)";

		Table T(UR"((1*)2*)");

		auto R = ProcessMatch(T.AsWrapper(), Str);

		if (R.has_value() && R->GetCaptureWrapper().HasSubCapture())
		{
			auto Capture = R->GetCaptureWrapper().GetTopSubCapture().GetCapture();
			if(Str.substr(Capture.Begin(), Capture.Count()) != UR"(1111)")
				throw UnpassedUnit{ "TestingReg : Bad Sample 5" };
		}else
			throw UnpassedUnit{ "TestingReg : Bad Sample 5" };
	}

	{
		std::u32string_view Str = UR"(11111)";

		Table T(UR"(1{1,7})");

		auto R = ProcessMatch(T.AsWrapper(), Str);

		if (!R.has_value())
			throw UnpassedUnit{ "TestingReg : Bad Sample 6" };
	}

	std::u32string_view Str = UR"(123456789abcdefABCDEF你好啊)";

	{
		Table T(UR"([a-zA-Z]+)");

		auto R = ProcessSearch(T.AsWrapper(), Str);

		if (!R.has_value() || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != UR"(abcdefABCDEF)")
			throw UnpassedUnit{ "TestingReg : Bad Sample 7" };
	}
	
	{

		std::u32string_view Str = UR"(abcdef{{}}ABCDEF你好啊)";
		Table T(UR"(\{[^\{\}]*\})");

		auto R = ProcessSearch(T.AsWrapper(), Str);

		auto Str2 = Str.substr(0, R->MainCapture.Begin());
		auto St3 = Str.substr(R->MainCapture.Begin(), R->MainCapture.Count());

		if (!R.has_value() || Str.substr(0, R->MainCapture.Begin()) != UR"(abcdef{)" || Str.substr(R->MainCapture.End()) != UR"(}ABCDEF你好啊)")
			throw UnpassedUnit{ "TestingReg : Bad Sample 9" };
	}


	{
		MulityRegexCreator Crerator;
		Crerator.AddRegex(UR"([1-4]+)", {1}, Crerator.GetCountedUniqueID());
		Crerator.AddRegex(UR"([1-9]+)", { 2 }, Crerator.GetCountedUniqueID());
		Crerator.AddRegex(UR"([a-z]+)", { 3 }, Crerator.GetCountedUniqueID());
		Crerator.AddRegex(UR"([A-Z]+)", { 4 }, Crerator.GetCountedUniqueID());
		Crerator.AddRegex(UR"((?:你|好|啊)+)", { 5 }, Crerator.GetCountedUniqueID());

		auto TBuffer = Crerator.Generate();

		std::vector<std::tuple<std::u32string_view, std::size_t>> List;

		auto IteStr = Str;

		while (!IteStr.empty())
		{
			auto R = ProcessGreedyFrontMatch(TableWrapper(TBuffer), IteStr);
			if (R.has_value())
			{
				List.push_back({ IteStr .substr(R->MainCapture.Begin(), R->MainCapture.Count()), R->AcceptData.Mask});
			}else
				throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
			IteStr = IteStr.substr(R->MainCapture.Count());
		}

		if (List.size() != 4)
			throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
		if (std::get<0>(List[0]) != UR"(123456789)" || std::get<1>(List[0]) != 2)
			throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
		if (std::get<0>(List[1]) != UR"(abcdef)" || std::get<1>(List[1]) != 3)
			throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
		if (std::get<0>(List[2]) != UR"(ABCDEF)" || std::get<1>(List[2]) != 4)
			throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
		if (std::get<0>(List[3]) != UR"(你好啊)" || std::get<1>(List[3]) != 5)
			throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
	}
	*/

	{
		MulityRegexCreator Crerator;
		Crerator.AddRegex(UR"((a){1,4})", { 2 }, Crerator.GetCountedUniqueID());
		//Crerator.AddRegex(UR"((ab){2,3}b)", { 2 }, Crerator.GetCountedUniqueID());
		//Crerator.AddRegex(UR"(+)", { 1 }, Crerator.GetCountedUniqueID(), true);
		

		auto TBuffer = Crerator.Generate();

		std::vector<std::tuple<std::u32string_view, std::size_t>> List;

		auto IteStr = UR"(1+21)";

		auto R = ProcessGreedyFrontMatch(TableWrapper(TBuffer), IteStr);


		volatile int i = 0;
	}

	std::wcout << LR"(TestingReg Pass !)" << std::endl;
}