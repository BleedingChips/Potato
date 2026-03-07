import PotatoFormat;
import PotatoEncode;
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

template<typename CharT, std::size_t N>
struct TypeString
{
	CharT string[N];
	constexpr std::basic_string_view<CharT> GetStringView() const { return { string }; }
	constexpr std::span<CharT const, N> GetSpan() const { return std::span{string}; }
	/*
	explicit consteval TypeString(const CharT(&str)[N]) : string{}
	{
		std::copy_n(str, N, string);
	}
	*/
	consteval TypeString(const CharT str[N]) : string{}
	{
		std::copy_n(str, N, string);
	}
	consteval TypeString(TypeString const& other)
	{
		std::copy_n(other.string, N, string);
	}

	template<typename CharT2, std::size_t N2>
	consteval bool operator==(TypeString<CharT2, N2> const& ref) const
	{
		if constexpr (std::is_same_v<CharT, CharT> && N == N2)
		{
			return std::equal(string, string + N, ref.string, N);
		}
		else
			return false;
	}

	constexpr std::size_t Size() const { return N; }
	using Type = CharT;
	static constexpr std::size_t Len = N;

	template<typename TargetT>
	constexpr std::size_t GetSize() const {
		auto info = Potato::Encode::StrEncoder<CharT, TargetT>::Encode(GetSpan(), {}, { false, true });
		return info.target_space;
	}

	template<typename TargetT>
	consteval auto CastTo() const
	{
		constexpr auto index = GetSize<TargetT>();
		TargetT Temp[index];
		Potato::Encode::StrEncoder<CharT, TargetT>::Encode(GetSpan(), std::span{ Temp }, { false, true });
		return TypeString(Temp);
	}

protected:
	TypeString() = default;
};

template<std::size_t N>
TypeString(const char32_t(&str)[N]) -> TypeString<char32_t, N>;

template<std::size_t N>
TypeString(const char16_t(&str)[N]) -> TypeString<char16_t, N>;

template<std::size_t N>
TypeString(const char8_t(&str)[N]) -> TypeString<char8_t, N>;

template<std::size_t N>
TypeString(const wchar_t(&str)[N]) -> TypeString<wchar_t, N>;

template<std::size_t N>
TypeString(const char(&str)[N]) -> TypeString<char, N>;


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

		constexpr auto info = Potato::Encode::StrEncoder<char8_t, wchar_t>{}.Encode(std::span(u8"abc"), {}, { false, true });
		
		constexpr auto str1 = TypeString(
			u8"nihaoa"
		);

		//constexpr auto str3 = str1.CastTo<char16_t>();

		volatile int o = 0;


		/*
		constexpr auto str2 = [](std::span<char8_t const> out) {
				constexpr auto info = Potato::Encode::StrEncoder<char8_t, wchar_t>{}.Encode(out, {}, { false, true });
				return 1;
			}(std::span(str1.string));
			*/

		volatile int i = 0;
		
	}
	catch (const char* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}
	return 0;
}