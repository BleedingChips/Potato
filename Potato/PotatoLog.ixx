export module PotatoLog;
import std;
import PotatoTMP;
import PotatoEncode;

export namespace Potato::Log
{
	enum class Level
	{
		Log,
	};

	template<std::size_t N>
	struct LogCategory : public TMP::TypeString<wchar_t, N>
	{
		using TMP::TypeString<wchar_t, N>::TypeString;
	};

	template<std::size_t N>
	LogCategory(const wchar_t(&str)[N]) -> Potato::Log::LogCategory<N>;
}

export namespace std
{
	template<typename CharT>
	struct formatter<Potato::Log::Level, CharT>
	{
		constexpr auto parse(std::basic_format_parse_context<CharT>& parse_context)
		{
			return parse_context.end();
		}
		template<typename FormatContext>
		constexpr auto format(Potato::Log::Level level, FormatContext& format_context) const
		{
			switch (level)
			{
			case Potato::Log::Level::Log:
				if constexpr (std::is_same_v<CharT, char>)
					return std::format_to(format_context.out(), "Log");
				else if constexpr (std::is_same_v<CharT, wchar_t>)
					return std::format_to(format_context.out(), L"Log");
				else
					static_assert(false);
				break;
			}
			return format_context.out();
		}
	};

	template<std::size_t N, typename CharT>
	struct formatter<Potato::Log::LogCategory<N>, CharT>
	{
		constexpr auto parse(std::basic_format_parse_context<CharT>& parse_context)
		{
			return parse_context.end();
		}
		template<typename FormatContext>
		constexpr auto format(Potato::Log::LogCategory<N> const& category, FormatContext& format_context) const
		{
			if constexpr (std::is_same_v<CharT, wchar_t>)
				return std::format_to(format_context.out(), L"{}", category.GetStringView());
			else
				static_assert(false);
			return format_context.out();
		}
	};
}



export namespace Potato::Log
{
	void DirectPrintWCout(std::wstring_view log);

	template<LogCategory category, Level Level, typename ...Pattern, typename ...Parameters>
	constexpr void Log(std::wformat_string<Pattern...>& pattern, Parameters&& ...parameters)
	{
		std::array<wchar_t, 1024> log_buffer;

		auto ite = log_buffer.begin();

		ite = std::format_to(
			ite,
			L"<{}:{}>:",
			category,
			Level
		);

		
		ite = std::format_to(
			ite,
			pattern.get(),
			std::forward<Parameters>(parameters)...
		);

		DirectPrintWCout(std::wstring_view{ log_buffer.data(), static_cast<std::size_t>(ite - log_buffer.begin())});

		//LogPrinter<Category, Level>{}.operator()<Pattern>(std::forward<Parameters>(parameters)...);
	}
}

