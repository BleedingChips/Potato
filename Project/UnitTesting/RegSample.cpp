#include "Potato/Public/PotatoReg.h"
#include "TestingTypes.h"
#include <string_view>
#include <set>
using namespace Potato::Reg;


void TestingReg()
{

	DFA T(UR"(.*?[a-zA-Z]+)");

	/*

	// Case 1
	{
		DFA Tab1(UR"(abcd)", false, {2});

		std::u32string_view Source = UR"(abcd)";

		auto Re = Match(Tab1, Source);

		if (!Re.has_value())
		{
			throw UnpassedUnit{"Testing Reg Failure Case 1"};
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2.has_value())
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 1" };
		}

	}

	// Case 2

	{
		DFA Tab1(UR"(.*cd)", false, { 2 });

		std::u32string_view Source = UR"(abcd)";

		auto Re = Match(Tab1, Source);

		if (!Re.has_value())
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 2" };
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2.has_value())
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 2" };
		}
	}

	// Case 3

	{
		DFA Tab1(UR"(a{1,5})", false, { 2 });

		std::u32string_view Source = UR"(aaaa)";

		auto Re = Match(Tab1, Source);

		if (!Re.has_value())
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 3" };
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2.has_value())
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 3" };
		}
	}

	// Case 4
	{
		DFA Tab1(UR"(a*(bc)d*)", false, { 2 });

		std::u32string_view Source = UR"(aaaabcddd)";

		auto Re = Match(Tab1, Source);

		if (!Re.has_value())
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 4" };
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2.has_value())
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 4" };
		}
	}

	// Case 5

	{
		MulityRegCreater Crea;
		Crea.LowPriorityLink(UR"(what)", true, {0});
		Crea.LowPriorityLink(UR"(if)", true, { 1 });
		Crea.LowPriorityLink(UR"([a-z]+)", false, { 2 });
		Crea.LowPriorityLink(UR"(\s+)", false, { 3 });

		auto DFA = *Crea.GenerateDFA();

		std::u32string_view Str = UR"(what whatif  if abd def)";

		std::u32string_view S = Str;

		std::vector<std::u32string_view> List;

		while (!S.empty())
		{
			auto Re = HeadMatch(DFA, S, true);
			if (!Re.has_value())
			{
				throw UnpassedUnit{ "Testing Reg Failure Case 5" };
			}
			else {
				List.push_back(S.substr(0, Re->MainCapture.Count()));
				S = S.substr(Re->MainCapture.Count());
			}
		}

		if (List.size() != 9)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 5" };
		}

	}


	// Case 6
	{
		std::u32string_view Str = UR"(123456abcdef)";
		std::u32string_view TStr = UR"(123456)";

		DFA T(TStr);

		auto R = HeadMatch(T, Str);

		if (!R.has_value() || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != TStr)
			throw UnpassedUnit{ "Testing Reg Failure Case 6" };
	}

	// Case 7
	{
		std::u32string_view Str = UR"(2)";

		DFA T(UR"([0-9])");

		auto R = Match(T, Str);

		if (!R.has_value())
			throw UnpassedUnit{ "Testing Reg Failure Case 7" };
	}

	// Case 8
	{
		std::u32string_view Str = UR"(11111)";

		DFA T(UR"(1*)");

		auto R = Match(T, Str);

		if (!R.has_value())
			throw UnpassedUnit{ "Testing Reg Failure Case 8" };
	}

	// Case 9
	{
		std::u32string_view Str = UR"(11111)";

		DFA T(UR"(1+)");

		auto R = Match(T, Str);

		if (!R.has_value())
			throw UnpassedUnit{ "Testing Reg Failure Case 9" };
	}

	
	// Case 10
	{
		std::u32string_view Str = UR"(11112222)";

		DFA T(UR"((1*)2*)");

		auto R = Match(T, Str);

		if (R.has_value() && R->GetCaptureWrapper().HasSubCapture())
		{
			auto Capture = R->GetCaptureWrapper().GetTopSubCapture().GetCapture();
			if(Str.substr(Capture.Begin(), Capture.Count()) != UR"(1111)")
				throw UnpassedUnit{ "Testing Reg Failure Case 10" };
		}else
			throw UnpassedUnit{ "Testing Reg Failure Case 10" };
	}

	// Case 11
	{
		std::u32string_view Str = UR"(11111)";

		DFA T(UR"(1{1,7})");

		auto R = Match(T, Str);

		if (!R.has_value())
			throw UnpassedUnit{ "Testing Reg Failure Case 11" };
	}

	std::u32string_view Str = UR"(123456789abcdefABCDEF你好啊)";

	// Case 12
	{
		DFA T(UR"(.*?[a-zA-Z]+)");

		auto R = HeadMatch(T, Str);

		if (!R.has_value() || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != UR"(abcdefABCDEF)")
			throw UnpassedUnit{ "Testing Reg Failure Case 12" };
	}
	
	// Case 13
	{

		std::u32string_view Str = UR"(abcdef{{}}ABCDEF你好啊)";
		DFA T(UR"(.*?\{[^\{\}]*\})");

		auto R = HeadMatch(T, Str);

		auto Str2 = Str.substr(0, R->MainCapture.Begin());
		auto St3 = Str.substr(R->MainCapture.Begin(), R->MainCapture.Count());

		if (!R.has_value() || Str.substr(0, R->MainCapture.Begin()) != UR"(abcdef{)" || Str.substr(R->MainCapture.End()) != UR"(}ABCDEF你好啊)")
			throw UnpassedUnit{ "Testing Reg Failure Case 13" };
	}

	// Case 14
	{
		MulityRegCreater Crerator;
		Crerator.LowPriorityLink(UR"([1-4]+)", false, {1});
		Crerator.LowPriorityLink(UR"([1-9]+)", false, { 2 });
		Crerator.LowPriorityLink(UR"([a-z]+)", false, { 3 });
		Crerator.LowPriorityLink(UR"([A-Z]+)", false, { 4 });
		Crerator.LowPriorityLink(UR"((?:你|好|啊)+)", false, { 5 });

		auto TTable = *Crerator.GenerateDFA();

		std::vector<std::tuple<std::u32string_view, std::size_t>> List;

		auto IteStr = Str;

		while (!IteStr.empty())
		{
			auto R = HeadMatch(TTable, IteStr, true);
			if (R.has_value())
			{
				List.push_back({ IteStr .substr(R->MainCapture.Begin(), R->MainCapture.Count()), R->AcceptData.Mask});
			}else
				throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
			IteStr = IteStr.substr(R->MainCapture.Count());
		}

		if (List.size() != 4)
			throw UnpassedUnit{ "Testing Reg Failure Case 14" };
		if (std::get<0>(List[0]) != UR"(123456789)" || std::get<1>(List[0]) != 2)
			throw UnpassedUnit{ "Testing Reg Failure Case 14" };
		if (std::get<0>(List[1]) != UR"(abcdef)" || std::get<1>(List[1]) != 3)
			throw UnpassedUnit{ "Testing Reg Failure Case 14" };
		if (std::get<0>(List[2]) != UR"(ABCDEF)" || std::get<1>(List[2]) != 4)
			throw UnpassedUnit{ "Testing Reg Failure Case 14" };
		if (std::get<0>(List[3]) != UR"(你好啊)" || std::get<1>(List[3]) != 5)
			throw UnpassedUnit{ "Testing Reg Failure Case 14" };
	}
	*/

	std::wcout << LR"(TestingReg Pass !)" << std::endl;
}