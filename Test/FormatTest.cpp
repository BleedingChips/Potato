import PotatoFormat;
import std;

using namespace Potato::Format;

/*
template<typename Target>
void TestScan(std::u8string_view Pattern, std::u8string_view Str, Target Tar, const char* Error)
{
	std::remove_cvref_t<Target> Tem;
	auto Re = MatchScan(Pattern, Str, Tem);
	if(!Re)
		throw Error;
	if(Tem != Tar)
		throw Error;
}

template<typename ...Target>
void TestFormat(std::u8string_view Pattern, std::u8string_view TarStr, const char* Error, Target&& ...Tar)
{
	auto FormatStr = FormatToString(Pattern, std::forward<Target>(Tar)...);
	if (!FormatStr.has_value() || FormatStr != TarStr)
	{
		throw Error;
	}
}


constexpr std::size_t Fund()
{
	FormatWritter<char8_t> Predict;
	auto K = Format(Predict, u8"abcedc");
	return Predict.GetWritedSize();
}

void TestingStrFormat()
{
	
	constexpr auto K = Fund();

	StaticFormatPattern<u8"abcd"> K2;

	auto P233 = StaticFormatToString<u8"aabcc">(1);

	//auto P = Formatt<u8"abc">();
	

	//constexpr std::u8string_view Str = u8"abc{}";
	//constexpr std::vector<char8_t> K = CreateFormatPatternImp(Str);


	TestScan<std::int32_t>(u8R"(([0-9]+))", u8R"(123455)", 123455, "StrFormatTest : Case 1");

	TestFormat(u8R"(123456{{}}{}{{)", u8R"(123456{}123455{)","StrFormatTest : Case 1",  123455);

	std::wcout << LR"(TestingStrFormat Pass !)" << std::endl;
}
*/
std::u8string_view format_string_bad1 = u8"sdasdasd}";


int main()
{
	try {
		{
			auto point1 = FormatterSyntax::FindSyntaxPoint(u8"sdasdasd}");
			if (point1.has_value())
			{
				throw "case 1";
			}
		}
		{
			auto point = FormatterSyntax::FindSyntaxPoint(u8"{sdasdasd");
			if (point.has_value())
			{
				throw "case 2";
			}
		}
		{
			auto point = FormatterSyntax::FindSyntaxPoint(u8"{{sdasdasd");
			if (!point.has_value())
			{
				throw "case 3";
			}
		}
		{
			auto point = FormatterSyntax::FindSyntaxPoint(u8"}}sdasdasd");
			if (!point.has_value())
			{
				throw "case 4";
			}
		}
		{
			auto point = FormatterSyntax::FindSyntaxPoint(u8"{1234}");
			if (!point.has_value())
			{
				throw "case 5";
			}
			if (point->string != u8"1234")
			{
				throw "case 5";
			}
		}

		StaticFormatPattern<u8"abc{{{123}}}abc{78} {{{56456}"> pattern;
		volatile int i = 0;
	}
	catch (const char* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}
	return 0;
}