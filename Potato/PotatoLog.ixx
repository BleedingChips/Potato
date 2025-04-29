export module PotatoLog;
import std;
import PotatoTMP;
import PotatoEncode;
import PotatoPointer;

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
				{
					auto str = std::string_view{"Log"};
					std::copy_n(str.data(), str.size(), format_context.out());
				}
				else if constexpr (std::is_same_v<CharT, wchar_t>)
				{
					auto str = std::wstring_view{ L"Log" };
					std::copy_n(str.data(), str.size(), format_context.out());
				}
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
			{
				auto str = category.GetStringView();
				std::copy_n(str.data(), str.size(), format_context.out());
			}
			return format_context.out();
		}
	};
}



export namespace Potato::Log
{

	std::pmr::memory_resource* LogMemoryResource();

	struct LogPrinter
	{
		struct Wrapper
		{
			void AddRef(LogPrinter const* ptr) { ptr->AddLogPrinterRef(); }
			void SubRef(LogPrinter const* ptr) { ptr->SubLogPrinterRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<LogPrinter, Wrapper>;

		virtual void Print(std::wstring_view print) = 0;

	protected:

		virtual void AddLogPrinterRef() const = 0;
		virtual void SubLogPrinterRef() const = 0;
	};

	LogPrinter::Ptr GetLogPrinter();

	template<LogCategory category>
	struct LogFormatter
	{
		template<typename OutputIte, typename ...Parameters>
		void operator()(OutputIte ite, Level level, std::wformat_string<std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
		{
			auto now = std::chrono::system_clock::now();
			//auto now_seconds = std::chrono::floor<std::chrono::seconds>(now);

			auto zoned_time = std::chrono::zoned_time{
				std::chrono::current_zone(),
				now
			};

			//auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - now_seconds);

			ite = std::format_to(
				ite,
				L"[{:%m.%d-%H:%M:%S}]<{}:{}>:",
				zoned_time,
				category,
				level
			);

			ite = std::format_to(
				ite,
				pattern,
				std::forward<Parameters>(parameters)...
			);
		}
	};


	template<LogCategory category, typename ...Parameters>
	constexpr void Log(Level level, std::wformat_string<std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
	{
		auto printer = GetLogPrinter();
		if (printer)
		{
			std::pmr::wstring output_str{ LogMemoryResource() };
			output_str.reserve(256);
			LogFormatter<category>{}(std::back_insert_iterator{ output_str }, level, pattern, std::forward<Parameters>(parameters)...);
			printer->Print(output_str);
		}
	}
}

