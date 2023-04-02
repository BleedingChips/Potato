import Potato.Reg;
using namespace Potato::Reg;

/*
bool TestMatch(std::u8string_view Reg, bool Raw, std::u8string_view Str, Accept Acce, const char* const Error)
{
		DFA Tab1(Reg, Raw, Acce);
		
		
		auto Re = Match(Tab1, Str);
		if (!Re || Re->AcceptData != Acce)
			return false;

		auto Tab2 = TableWrapper::Create(Tab1);
		
		auto Re2 = Match(TableWrapper{Tab2}, Str);
		
		if (!Re2 || Re2->AcceptData != Acce)
			return false;
}

void TestMatchCapture(std::u8string_view Reg, std::u8string_view Str, std::u8string_view RequireStr, const char* Error)
{

	auto Tab1 = TableWrapper::Create(Reg);

	auto Re = HeadMatch(TableWrapper{Tab1}, Str);
	if (!Re || !Re->GetCaptureWrapper().HasSubCapture() || Re->GetCaptureWrapper().GetTopSubCapture().Slice(Str) != RequireStr)
		throw Error;
}

void TestHeadMatch(std::u8string_view Reg, bool Raw, std::u8string_view Str, std::u8string_view RequireStr, Accept Acce, const char* const Error)
{
	try {
		DFA Tab1(Reg, Raw, Acce);

		auto Re = HeadMatch(Tab1, Str);
		if (!Re || Re->GetCaptureWrapper().Slice(Str) != RequireStr || Re->AcceptData != Acce)
			throw Error;

		auto Tab2 = TableWrapper::Create(Tab1);

		auto Re2 = Match(TableWrapper{ Tab2 }, Str);

		if (!Re2 || Re->GetCaptureWrapper().Slice(Str) != RequireStr || Re->AcceptData != Acce)
			throw Error;
	}
	catch (Exception::Interface const&)
	{
		throw Error;
	}
}

void TestGreddyHeadMatch(const char* const Error)
{
	try
	{
		MulityRegCreater Crerator;
		Crerator.LowPriorityLink(u8R"(\+)", false, { 2 });
		Crerator.LowPriorityLink(u8R"(\s)", false, { 3 });
		Crerator.LowPriorityLink(u8R"([0-9]+)", false, { 4 });
		Crerator.LowPriorityLink(u8R"(while)", false, { 5 });
		Crerator.LowPriorityLink(u8R"([a-zA-Z][a-z0-9A-Z]*)", false, { 1 });
		auto Table = Crerator.GenerateDFA();


		std::u8string_view Source = u8R"(abc abc while 12 + while123)";

		struct Enum
		{
			std::size_t Mask;
			std::u8string_view Str;
			bool operator==(Enum const& I2) const{ return Mask == I2.Mask && Str == I2.Str; }
		};

		std::vector<Enum> TarEnums = {
			{1, u8R"(abc)"},
			{3, u8R"( )"},
			{1, u8R"(abc)"},
			{3, u8R"( )"},
			{5, u8R"(while)"},
			{3, u8R"( )"},
			{4, u8R"(12)"},
			{3, u8R"( )"},
			{2, u8R"(+)"},
			{3, u8R"( )"},
			{1, u8R"(while123)"}
		};

		auto TTable = *Crerator.GenerateDFA();

		{
			std::vector<Enum> CurI;
			auto Ite = Source;
			HeadMatchProcessor Pro(TTable, true);
			while (!Ite.empty())
			{
				Pro.Clear();
				auto Re = HeadMatch(Pro, Ite);
				if(!Re)
					throw Error;
				CurI.push_back({Re->AcceptData.Mask, Re->GetCaptureWrapper().Slice(Ite)});
				Ite = Ite.substr(Re->GetCaptureWrapper().Count());
			}
			if(CurI != TarEnums)
				throw Error;
		}
	}
	catch (Exception::Interface const&)
	{
		throw Error;
	}
}



void TestingReg()
{

	Accept DefaultAcc = {100, 2007};

	TestMatch(u8R"(abcd)", false, u8R"(abcd)", DefaultAcc, "TestingReg:: Case 1");
	TestMatch(u8R"({})", true, u8R"({})", DefaultAcc, "TestingReg:: Case 2");
	TestMatch(u8R"([0-9])", false, u8R"(8)", DefaultAcc, "TestingReg:: Case 3");
	TestMatch(u8R"([^0-9])", false, u8R"(a)", DefaultAcc, "TestingReg:: Case 4");
	TestMatch(u8R"(a|b)", false, u8R"(a)", DefaultAcc, "TestingReg:: Case 5");
	TestMatch(u8R"(a|b)", false, u8R"(b)", DefaultAcc, "TestingReg:: Case 6");
	TestMatch(u8R"(a*)", false, u8R"(aaa)", DefaultAcc, "TestingReg:: Case 7");
	TestMatch(u8R"(a+)", false, u8R"(aaa)", DefaultAcc, "TestingReg:: Case 8");
	TestMatch(u8R"(ba?)", false, u8R"(b)", DefaultAcc, "TestingReg:: Case 9");
	TestMatch(u8R"(ba?)", false, u8R"(ba)", DefaultAcc, "TestingReg:: Case 10");
	TestMatch(u8R"((ba?))", false, u8R"(ba)", DefaultAcc, "TestingReg:: Case 11");
	TestMatch(u8R"((?:ba?))", false, u8R"(ba)", DefaultAcc, "TestingReg:: Case 12");
	TestMatch(u8R"((?:ba)?)", false, u8R"(ba)", DefaultAcc, "TestingReg:: Case 13");
	TestMatch(u8R"(a{0,4})", false, u8R"(aaa)", DefaultAcc, "TestingReg:: Case 14");
	TestMatch(u8R"(a{4})", false, u8R"(aaaa)", DefaultAcc, "TestingReg:: Case 15");
	TestMatch(u8R"(a{4,})", false, u8R"(aaaa)", DefaultAcc, "TestingReg:: Case 16");
	TestMatch(u8R"(a{,4})", false, u8R"(aaaa)", DefaultAcc, "TestingReg:: Case 17");

	TestHeadMatch(u8R"(bca*)", false, u8R"(bcaaaa)", u8R"(bcaaaa)", DefaultAcc, "TestingReg:: Case 18");
	TestHeadMatch(u8R"(bca*?)", false, u8R"(bcaaaa)", u8R"(bc)", DefaultAcc, "TestingReg:: Case 19");

	TestGreddyHeadMatch("TestingReg:: Case 20");

	TestMatchCapture(u8R"(([0-9]+))", u8R"(123456)", u8R"(123456)", "TestingReg:: Case 21");

	std::wcout << LR"(TestingReg Pass !)" << std::endl;
}
*/

int main()
{

	try {
		NfaT N(U"abs\\d123-123[a]");
		volatile int i = 0;
	}
	catch (Exception::UnaccaptableRegex const& Rex)
	{
		std::wstring_view Last = std::wstring_view{ Rex.TotalString }.substr(Rex.BadIndex.Begin(), Rex.BadIndex.End());
		volatile int i = 0;
	}
	

	/*
	try {
		TestingReg();
	}
	catch (char const* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}
	*/
	return 0;
}