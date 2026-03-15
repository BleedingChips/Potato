import PotatoFormat;
import PotatoEncode;
import PotatoTMP;
import std;

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

void TestingStrFormat()
{

	//auto P = Formatt<u8"abc">();
	

	//constexpr std::u8string_view Str = u8"abc{}";
	//constexpr std::vector<char8_t> K = CreateFormatPatternImp(Str);


	TestScan<std::int32_t>(u8R"(([0-9]+))", u8R"(123455)", 123455, "StrFormatTest : Case 1");

	//TestFormat(u8R"(123456{{}}{}{{)", u8R"(123456{}123455{)","StrFormatTest : Case 1",  123455);

	std::wcout << LR"(TestingStrFormat Pass !)" << std::endl;
}

struct K
{

};

namespace Potato::Format
{
	template<typename CharT>
	struct Formatter<K, CharT>
	{
		std::u8string_view texts = u8"sads";
		constexpr Formatter(std::basic_string_view<CharT> syntax) : texts(syntax)
		{
		};
		template<typename Iterator>
		Iterator Format(Iterator iterator, K const&) const
		{
			std::u8string_view cc = u8"<class K>";
			return std::copy_n(
				cc.data(),
				cc.size(),
				std::move(iterator)
			);
		}
	};
}

struct Ref
{
	float k;
};


namespace std
{

	template<>
	struct formatter<Ref, char> : formatter<float, char>
	{
		constexpr auto parse(std::basic_format_parse_context<char>& parse_context)
		{
			auto p = formatter<float, char>::parse(parse_context);
			return p;
		}

		template<typename Con>
		auto format(Ref f, Con&& con) const
		{
			return formatter<float, char>::format(f.k, con);
		}
	};

	template<>
	struct formatter<K, char>
	{

		constexpr auto parse(std::basic_format_parse_context<char>& parse_context)
		{
			std::size_t index = 1;
			for (auto ite = parse_context.begin(); ite != parse_context.end(); ++ite)
			{
				if (*ite == '{')
				{
					index += 1;
				}
				if (*ite == '}')
				{
					index -= 1;
					if (index == 0)
						return ite;
				}
			}
			return parse_context.end();
		}

		template<typename Con>
		auto format(K , Con&& con) const
		{
			std::string_view str = "<class K>";
			return std::copy_n(
				str.data(),
				str.size(),
				con.out()
			);
		}
	};
}

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
			auto point = FormatterSyntax::FindSyntaxPoint(u8"{124:1234abc}");
			if (!point.has_value())
			{
				throw "case 5";
			}
			if (point->string != u8"1234abc")
			{
				throw "case 5";
			}
		}

		{
			StaticFormatPattern<u8"abc {:abc} {:bcd}"> pattern;
			std::u8string p;
			K k;
			pattern.Format(std::back_inserter(p), k, k, k);
			volatile int i = 0;
		}
		
		{
			StaticFormatPattern<"abc {:*6} {:*6}"> pattern;
			std::string p;
			K k;
			pattern.Format(std::back_inserter(p), 1, 2);
			volatile int i = 0;
		}
		
		{
			K k;
			std::string pk;
			std::format_to(std::back_insert_iterator(pk), "{:abc{}} {:} {:} {:}", k, 1, k, k, k, k);

			volatile int i = 0;
		}

		{
			K k;
			std::string pk;
			std::format_to(std::back_insert_iterator(pk), "{} {:{}.{}f} {} {}", 10086, Ref{100000.0f}, 1, 2, 3, 4);

			volatile int i = 0;
		}

		{

			std::int32_t index = 0;
			float tar = 0;
			auto info = Deformat<u8".*Loc:\\[{}\\.\\] Rot:\\[{}">(std::u8string_view{ u8"sdajlsdkasjdfaLoc:[ 1234.] Rot:[-124.00434]" }, index, tar);

			if (index != 1234 && !(tar < -1234.0f && tar > -1235.0f))
			{
				throw "case 6";
			}

			volatile int i = 0;
		}

		volatile int o = 0;

		volatile int i = 0;
		
	}
	catch (const char* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}

	return 0;
}