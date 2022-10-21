#include "Potato/Public/PotatoStrFormat.h"
#include "TestingTypes.h"

using namespace Potato::StrFormat;


template<typename Target>
void TestScan(std::u8string_view Pattern, std::u8string_view Str, Target Tar, const char* Error)
{
	std::remove_cvref_t<Target> Tem;
	auto Re = MatchScan(Pattern, Str, Tem);
	if(!Re)
		throw UnpassedUnit{Error};
	if(Tem != Tar)
		throw UnpassedUnit{ Error };
}

template<typename ...Target>
void TestFormat(std::u8string_view Pattern, std::u8string_view TarStr, const char* Error, Target&& ...Tar)
{
	auto FormatStr = FormatTo(Pattern, std::forward<Target>(Tar)...);
	if (!FormatStr.has_value() || FormatStr != TarStr)
	{
		throw UnpassedUnit{ Error };
	}
}


void TestingStrFormat()
{

	TestScan<int32_t>(u8R"(([0-9]+))", u8R"(123455)", 123455, "StrFormatTest : Case 1");

	TestFormat(u8R"(123456{{}}{}{{)", u8R"(123456{}123455{)","StrFormatTest : Case 1",  123455);

	std::wcout << LR"(TestingStrFormat Pass !)" << std::endl;
}