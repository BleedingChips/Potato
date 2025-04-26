export module PotatoLog;
import std;
import PotatoTMP;
export import PotatoFormat;

export namespace Potato::Log
{
	enum class Level
	{
		Log,
	};

	template<std::size_t N>
	struct TypeStringW : public TMP::TypeString<wchar_t, N>
	{
		using TMP::TypeString<wchar_t, N>::TypeString;
	};

	template<std::size_t N>
	TypeStringW(const wchar_t(&str)[N]) -> Potato::Log::TypeStringW<N>;
}

export namespace Potato::Format
{
	export template<>
	struct Formatter<Log::Level, char8_t>
	{
		bool operator()(Format::FormatWriter<Log::Level>& Writer, std::basic_string_view<char8_t> Parameter, Log::Level const& Input);
	};

	
}



export namespace Potato::Log
{

	



	struct TerminalOutput
	{
		void operator()(std::wstring_view output);
	};

	struct DefaultPrinter
	{
		template<TypeStringW Category, Level Level, TypeStringW Pattern, typename ...Parameters>
		void operator()(Parameters&& ...parameters)
		{
			auto str = Format::StaticFormatToString<Pattern>(std::forward<Parameters>(parameters)...);
			if(str)
			{
				TerminalOutput{}(*str);
			}
		}
	};

	template<TypeStringW Category, Level Level>
	struct LogPrinter
	{
		template<TypeStringW Pattern, typename ...Parameters>
		void operator()(Parameters&& ...parameters)
		{
			DefaultPrinter{}.operator()< Category, Level, Pattern >(std::forward<Parameters>(parameters)...);
		}
	};

	template<TypeStringW Category, Level Level, TypeStringW Pattern, typename ...Parameters>
	void Log(Parameters&& ...parameters)
	{
		LogPrinter<Category, Level>{}.operator()<Pattern>(std::forward<Parameters>(parameters)...);
	}

	template<TypeStringW Category, Level Level, TypeStringW Pattern>
	void Log()
	{
		LogPrinter<Category, Level>{}.operator() < Pattern > ();
	}
}

