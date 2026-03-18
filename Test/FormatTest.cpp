import PotatoFormat;
import PotatoEncode;
import PotatoTMP;
import std;

using namespace Potato::Format;


struct K
{

};

namespace Potato::Format
{
	
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
			std::u8string_view str;
			std::u8string str2;
			std::wstring str3;
			auto iterator_cc = std::back_inserter(str2);
			auto info = Deformat<u8".*?Loc:\\[{}\\.\\] Rot:\\[{}.*?\\{{\\}}\\{{\\}}\\{{\\}}">(std::u8string_view{ u8"sdajlsdkasjdfaLoc:[ 1234.] Rot:[-124.00434]sdasdasd{abcde}{123456}{123abcdef}" }, index, tar, str, iterator_cc, str3);

			if (index != 1234  || !(tar < -123.5f && tar > -125.0f) || str != u8"abcde" || str2 != u8"123456")
			{
				throw "case 6";
			}

			std::size_t indexcc = 0;

			auto infocc = DirectDeformat(u8"123456", indexcc);

			

			volatile int i = 0;
		}
		
	}
	catch (const char* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}

	return 0;
}