#include "Potato/Public/PotatoStrEncode.h"
#include "TestingTypes.h"
#include <iostream>

using namespace Potato::StrEncode;

template<typename ST, typename TT>
struct StrEncodeTesting
{
	bool operator()(std::basic_string_view<ST> Source, std::basic_string_view<TT> Target)
	{
		EncodeInfo I1 = StrEncoder<ST, TT>::RequireSpace(Source);
		std::basic_string<TT> R1;
		R1.resize(I1.TargetSpace);
		StrEncoder<ST, TT>::EncodeUnSafe(Source, R1, I1.CharacterCount);
		return R1 == Target;
	}
};


void TestingStrEncode()
{

	std::u32string_view TestingStr1 = UR"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";
	std::u16string_view TestingStr2 = uR"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";
	std::u8string_view TestingStr3 = u8R"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";
	std::wstring_view TestingStr4 = LR"(𐍉𐌱这是一个标准测试一二三䍚《》？？？᠀This Is A Standard Testing<>??abcdeASDF12309)";

	if (!StrEncodeTesting<char32_t, char8_t>{}(TestingStr1, TestingStr3))
		throw UnpassedUnit{ "TestingStrEncode : UTF32 To UTF8" };
	if (!StrEncodeTesting<char32_t, char16_t>{}(TestingStr1, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : UTF32 To UTF16" };
	if (!StrEncodeTesting<char32_t, wchar_t>{}(TestingStr1, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : UTF32 To wchar_t" };
	if (!StrEncodeTesting<char32_t, char32_t>{}(TestingStr1, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : UTF32 To UTF32" };
	if (!StrEncodeTesting<char16_t, char32_t>{}(TestingStr2, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To UTF32" };
	if (!StrEncodeTesting<char16_t, char16_t>{}(TestingStr2, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To UTF16" };
	if (!StrEncodeTesting<char16_t, char8_t>{}(TestingStr2, TestingStr3))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To UTF8" };
	if (!StrEncodeTesting<char16_t, wchar_t>{}(TestingStr2, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : UTF16 To wchar_t" };
	if (!StrEncodeTesting<char8_t, char32_t>{}(TestingStr3, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To UTF32" };
	if (!StrEncodeTesting<char8_t, char16_t>{}(TestingStr3, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To UTF16" };
	if (!StrEncodeTesting<char8_t, char8_t>{}(TestingStr3, TestingStr3))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To UTF8" };
	if (!StrEncodeTesting<char8_t, wchar_t>{}(TestingStr3, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : UTF8 To wchar_t" };
	if (!StrEncodeTesting<wchar_t, char32_t>{}(TestingStr4, TestingStr1))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To UTF32" };
	if (!StrEncodeTesting<wchar_t, char16_t>{}(TestingStr4, TestingStr2))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To UTF16" };
	if (!StrEncodeTesting<wchar_t, char8_t>{}(TestingStr4, TestingStr3))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To UTF8" };
	if (!StrEncodeTesting<wchar_t, wchar_t>{}(TestingStr4, TestingStr4))
		throw UnpassedUnit{ "TestingStrEncode : wchar_t To wchar_t" };

	std::cout << R"(TestingStrEncode Pass !)" << std::endl;
}