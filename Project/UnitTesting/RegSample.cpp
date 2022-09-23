#include "Potato/Public/PotatoReg.h"
#include "TestingTypes.h"
#include <string_view>
#include <set>
using namespace Potato::Reg;


void TestingReg()
{


	// Case 1
	{
		DFA Tab1(u8R"(abcd)", false, {2});

		std::u8string_view Source = u8R"(abcd)";

		auto Re = Match(Tab1, Source);

		if (!Re)
		{
			throw UnpassedUnit{"Testing Reg Failure Case 1"};
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 1" };
		}

	}

	// Case 2

	{
		DFA Tab1(u8R"(.*cd)", false, { 2 });

		std::u8string_view Source = u8R"(abcd)";

		auto Re = Match(Tab1, Source);

		if (!Re)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 2" };
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 2" };
		}
	}

	// Case 3

	{
		DFA Tab1(u8R"(a{1,5})", false, { 2 });

		std::u8string_view Source = u8R"(aaaa)";

		auto Re = Match(Tab1, Source);

		if (!Re)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 3" };
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 3" };
		}
	}

	// Case 4
	{
		DFA Tab1(u8R"(a*(bc)d*)", false, { 2 });

		std::u8string_view Source = u8R"(aaaabcddd)";

		auto Re = Match(Tab1, Source);

		if (!Re)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 4" };
		}

		auto Wra = TableWrapper::Create(Tab1);

		auto Re2 = Match(Tab1, Source);

		if (!Re2)
		{
			throw UnpassedUnit{ "Testing Reg Failure Case 4" };
		}
	}

	// Case 5

	{
		MulityRegCreater Crea;
		Crea.LowPriorityLink(u8R"(what)", true, {0});
		Crea.LowPriorityLink(u8R"(if)", true, { 1 });
		Crea.LowPriorityLink(u8R"([a-z]+)", false, { 2 });
		Crea.LowPriorityLink(u8R"(\s+)", false, { 3 });

		auto DFA = *Crea.GenerateDFA();

		std::u8string_view Str = u8R"(what whatif  if abd def)";

		std::u8string_view S = Str;

		std::vector<std::u8string_view> List;

		while (!S.empty())
		{
			auto Re = HeadMatch(DFA, S, true);
			if (!Re)
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
		std::u8string_view Str = u8R"(123456abcdef)";
		std::u8string_view TStr = u8R"(123456)";

		DFA T(TStr);

		auto R = HeadMatch(T, Str);

		if (!R || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != TStr)
			throw UnpassedUnit{ "Testing Reg Failure Case 6" };
	}

	// Case 7
	{
		std::u8string_view Str = u8R"(2)";

		DFA T(u8R"([0-9])");

		auto R = Match(T, Str);

		if (!R)
			throw UnpassedUnit{ "Testing Reg Failure Case 7" };
	}

	// Case 8
	{
		std::u8string_view Str = u8R"(11111)";

		DFA T(u8R"(1*)");

		auto R = Match(T, Str);

		if (!R)
			throw UnpassedUnit{ "Testing Reg Failure Case 8" };
	}

	// Case 9
	{
		std::u8string_view Str = u8R"(11111)";

		DFA T(u8R"(1+)");

		auto R = Match(T, Str);

		if (!R)
			throw UnpassedUnit{ "Testing Reg Failure Case 9" };
	}

	
	// Case 10
	{
		std::u8string_view Str = u8R"(11112222)";

		DFA T(u8R"((1*)2*)");

		auto R = Match(T, Str);

		if (R && R->GetCaptureWrapper().HasSubCapture())
		{
			auto Capture = R->GetCaptureWrapper().GetTopSubCapture().GetCapture();
			if(Str.substr(Capture.Begin(), Capture.Count()) != u8R"(1111)")
				throw UnpassedUnit{ "Testing Reg Failure Case 10" };
		}else
			throw UnpassedUnit{ "Testing Reg Failure Case 10" };
	}

	// Case 11
	{
		std::u8string_view Str = u8R"(11111)";

		DFA T(u8R"(1{1,7})");

		auto R = Match(T, Str);

		if (!R)
			throw UnpassedUnit{ "Testing Reg Failure Case 11" };
	}

	std::u8string_view Str = u8R"(123456789abcdefABCDEF你好啊)";

	// Case 12
	{
		DFA T(u8R"(.*?[a-zA-Z]+)");

		auto R = HeadMatch(T, Str);

		if (!R || Str.substr(R->MainCapture.Begin(), R->MainCapture.Count()) != u8R"(123456789abcdefABCDEF)")
			throw UnpassedUnit{ "Testing Reg Failure Case 12" };
	}

	// Case 13
	{
		MulityRegCreater Crerator;
		Crerator.LowPriorityLink(u8R"([1-4]+)", false, {1});
		Crerator.LowPriorityLink(u8R"([1-9]+)", false, { 2 });
		Crerator.LowPriorityLink(u8R"([a-z]+)", false, { 3 });
		Crerator.LowPriorityLink(u8R"([A-Z]+)", false, { 4 });
		Crerator.LowPriorityLink(u8R"((?:你|好|啊)+)", false, { 5 });

		auto TTable = *Crerator.GenerateDFA();

		std::vector<std::tuple<std::u8string_view, std::size_t>> List;

		auto IteStr = Str;

		while (!IteStr.empty())
		{
			auto R = HeadMatch(TTable, IteStr, true);
			if (R)
			{
				List.push_back({ IteStr .substr(R->MainCapture.Begin(), R->MainCapture.Count()), R->AcceptData.Mask});
			}else
				throw UnpassedUnit{ "TestingReg : Bad Sample 8" };
			IteStr = IteStr.substr(R->MainCapture.Count());
		}

		if (List.size() != 4)
			throw UnpassedUnit{ "Testing Reg Failure Case 13" };
		if (std::get<0>(List[0]) != u8R"(123456789)" || std::get<1>(List[0]) != 2)
			throw UnpassedUnit{ "Testing Reg Failure Case 13" };
		if (std::get<0>(List[1]) != u8R"(abcdef)" || std::get<1>(List[1]) != 3)
			throw UnpassedUnit{ "Testing Reg Failure Case 13" };
		if (std::get<0>(List[2]) != u8R"(ABCDEF)" || std::get<1>(List[2]) != 4)
			throw UnpassedUnit{ "Testing Reg Failure Case 13" };
		if (std::get<0>(List[3]) != u8R"(你好啊)" || std::get<1>(List[3]) != 5)
			throw UnpassedUnit{ "Testing Reg Failure Case 13" };
	}

	{
		DFA T(u8R"(\'([^\s]+)\')");

		std::u8string_view Str = u8R"('a')";

		auto Re = HeadMatch(T, Str, true);

		volatile int i = 0;

	}


	std::wcout << LR"(TestingReg Pass !)" << std::endl;
}