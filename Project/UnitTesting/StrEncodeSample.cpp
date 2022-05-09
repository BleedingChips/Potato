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

	/*
	DocumentReader Reader(L"C:/Sample.txt");
	if(Reader)
	{
		// 成功打开
		std::vector<std::byte> Tembuffer;
		// 直接分配空间储存所有二进制数据
		Tembuffer.resize(Reader.RecalculateLastSize());
		// 根据缓冲区创建Wrapper，注意，此时Wrapper关联了Tembuffer，所有Tembuffer的扩缩容或者其他改变储存地址的操作都会使Wrapper的操作崩溃。
		DocumenetReaderWrapper Wrapper = Reader.CreateWrapper(Tembuffer);
		// Reader 将数据写入 Wrapper 中。注意，写入的数据量与 Wrapper 关联的内存大小和内容有直接关系。
		// 该操作不保证会将所有数据写入，除非Wrapper能保证其内部的缓存空间大于或等于文件内的所有数据，并且Wrapper内部的指针位于开头。
		FlushResult FR = Reader.Flush(Wrapper);

		while(Wrapper)
		{
			std::u32string ReadBuffer;
			// 消耗Wrapper内的数据，直到遇上换行符。
			// 并将数据转成char32_t的格式写入ReadBuffer中，该操作只会往ReadBuffer的末尾添加数据。
			Wrapper.ReadLine(ReadBuffer);
			// ...
		}
	}
	*/
	/*
	// 重新打开文件，写入UTF8的BOM，若文件已存在，则覆盖
	DocumentWriter Writer(L"C:/Sample.txt", DocumenetBomT::UTF8);
	std::u32string_view Str = U"12344";
	// 将字符串转换成 DocumenetBomT 所对应的格式，写入文件的末尾。
	Writer.Write(Str);
	*/

	std::cout << R"(TestingStrEncode Pass !)" << std::endl;
}