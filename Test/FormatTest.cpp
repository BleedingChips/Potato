import Potato.Format;

using namespace Potato::Format;


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
	auto FormatStr = Format(Pattern, std::forward<Target>(Tar)...);
	if (!FormatStr.has_value() || FormatStr != TarStr)
	{
		throw Error;
	}
}


void TestingStrFormat()
{

	constexpr auto K = FormatSize(u8"abcedc");

	StaticFormatPattern<u8"abcd"> K2;

	constexpr auto K6 = decltype(K2)::FormatSize();

	auto P233 = StaticFormat<u8"aabcc">(1);

	//auto P = Formatt<u8"abc">();
	

	//constexpr std::u8string_view Str = u8"abc{}";
	//constexpr std::vector<char8_t> K = CreateFormatPatternImp(Str);


	TestScan<int32_t>(u8R"(([0-9]+))", u8R"(123455)", 123455, "StrFormatTest : Case 1");

	TestFormat(u8R"(123456{{}}{}{{)", u8R"(123456{}123455{)","StrFormatTest : Case 1",  123455);

	std::wcout << LR"(TestingStrFormat Pass !)" << std::endl;
}

int main()
{
	try {
		TestingStrFormat();
	}
	catch (const char* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}
	return 0;
}