#include "Potato/Public/PotatoStrFormat.h"
#include "TestingTypes.h"

using namespace Potato::StrFormat;


void TestingStrFormat()
{

	

	FormatPattern<char32_t> Pattern(UR"(12345{}6789{{}})");
	std::u32string Output;
	int32_t Data = 10086;
	uint64_t Data2 = 10081;
	Pattern.Format(Output, Data, Data2);

	if (Output != UR"(12345100866789{10081})")
		throw UnpassedUnit{ "TestingStrFormat : Bad Format Output 0" };

	std::u32string_view Str = UR"(sdasdasd12445sdasdasd)";

	int32_t R1 = 0;

	SearchScan(UR"(([0-9]+))", Str, R1);

	if (R1 != 12445)
		throw UnpassedUnit{ "TestingStrFormat : Bad SearchScan Output 1" };

	uint64_t R2 = 0;

	MatchScan(UR"(sdasdasd([0-9]+)sdasdasd)", Str, R2);

	if (R2 != 12445)
		throw UnpassedUnit{ "TestingStrFormat : Bad SearchScan Output 2" };

	std::wcout << LR"(TestingStrFormat Pass !)" << std::endl;
}