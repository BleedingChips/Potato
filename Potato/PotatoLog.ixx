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
	struct LogCategory : public TMP::TypeString<char8_t, N>
	{
		using TMP::TypeString<char8_t, N>::TypeString;
	};

	template<std::size_t N>
	LogCategory(const char8_t(&str)[N]) -> LogCategory<N>;
}

export namespace std
{
	template<>
	struct formatter<Potato::Log::Level, char>
	{
		constexpr auto parse(std::basic_format_parse_context<char>& parse_context)
		{
			return parse_context.begin();
		}

		template<typename FormatContext>
		constexpr auto format(Potato::Log::Level level, FormatContext& format_context) const
		{
			switch (level)
			{
			case Potato::Log::Level::Log:
			{
				constexpr std::string_view str = u8"Log";
				auto out_ite = format_context.out();
				for (auto ite : str)
				{
					*(out_ite++) = ite;
				}
				return out_ite;
			}
			default:
				return format_context.out();
			}
		}
	};

	template<std::size_t N>
	struct formatter<Potato::Log::LogCategory<N>, char>
	{
		constexpr auto parse(std::basic_format_parse_context<char>& parse_context)
		{
			return parse_context.begin();
		}
		template<typename FormatContext>
		constexpr auto format(Potato::Log::LogCategory<N> const& category, FormatContext& format_context) const
		{
			auto str = category.GetStringView();
			auto out_ite = format_context.out();
			for (auto ite : str)
			{
				*(out_ite++) = ite;
			}
			return out_ite;
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

		virtual void Print(std::u8string_view print) = 0;

	protected:

		virtual void AddLogPrinterRef() const = 0;
		virtual void SubLogPrinterRef() const = 0;
	};

	LogPrinter::Ptr GetLogPrinter();

	struct DefaultLogFormatter;

	template<LogCategory category>
	struct LogFormatter
	{
		template<typename OutputIte, typename ...Parameters>
		void operator()(OutputIte ite, Level level, std::basic_format_string<char8_t, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
		{
			auto now = std::chrono::system_clock::now();
			auto second_now = std::chrono::floor<std::chrono::seconds>(now);
			
			auto zoned_time = std::chrono::zoned_time{
				std::chrono::current_zone(),
				second_now
			};

			auto mil_second = std::chrono::floor<std::chrono::milliseconds>(now - second_now);

			ite = std::format_to(
				ite,
				"[{:%m.%d-%H:%M:%S}.{:0>3}]<{}><{}>",
				zoned_time,
				mil_second.count(),
				category,
				level
			);

			ite = std::format_to(
				ite,
				pattern,
				std::forward<Parameters>(parameters)...
			);

			*ite++ = '\n';
		}
	};
	/*
	template<typename ...Parameters>
	struct FormatStringWrapper : std::format_string<std::type_identity_t<Parameters>...>
	{
		consteval FormatStringWrapper(std::basic_string_view<char8_t> str)
			: std::format_string<std::type_identity_t<Parameters>...>(std::string_view{ reinterpret_cast<char const*>(str.data()), str.size() })
		{

		}
		template<std::size_t N>
		consteval FormatStringWrapper(const char8_t(&str)[N])
			: std::format_string<std::type_identity_t<Parameters>...>(reinterpret_cast<const char& [N]>(str))
		{

		}
		FormatStringWrapper(FormatStringWrapper const&) = default;
	};
	*/

	template<LogCategory category, typename ...Parameters>
	constexpr void Log(Level level, std::u8string_view pattern, Parameters&& ...parameters)
	{
		/*
		auto printer = GetLogPrinter();
		if (printer)
		{
			std::pmr::string output_str{ LogMemoryResource() };
			LogFormatter<category>{}(std::back_insert_iterator{ output_str }, level, pattern, std::forward<Parameters>(parameters)...);
			std::u8string_view output_view = { reinterpret_cast<char8_t const*>(output_str.data()), output_str.size() };
			printer->Print(output_view);
		}
		*/
	}
}

