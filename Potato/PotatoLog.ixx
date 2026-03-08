export module PotatoLog;
import std;
import PotatoTMP;
import PotatoEncode;
import PotatoPointer;

export namespace Potato::Log
{
	enum class LogLevel
	{
		VeryVerbose,
		Verbose,
		Log,
		Display, 
		Warning,
		Error
	};

	template<TMP::TypeString>
	struct LogCategory
	{
	};

	template<std::size_t N>
	LogCategory(const char8_t(&str)[N]) -> LogCategory<TMP::TypeString(str)>;

	struct FormatedSystemTime
	{
		std::chrono::system_clock::time_point current_time;

		FormatedSystemTime(std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now())
			:current_time(current_time)
		{
		}
	};

	struct LogLine
	{
		FormatedSystemTime current_time;
		std::u8string_view category;
		LogLevel level;
		std::u8string_view log_message;
	};

	
}

export namespace std
{
	template<>
	struct formatter<Potato::Log::LogLevel, wchar_t>
	{
		constexpr auto parse(std::basic_format_parse_context<char>& parse_context)
		{
			if(*parse_context.begin() == L'}')
				return parse_context.begin();
			else
				throw "LogLevel do not support any parameter";
		}

		template<typename FormatContext>
		constexpr auto format(Potato::Log::LogLevel level, FormatContext& format_context) const
		{
			switch (level)
			{
			case Potato::Log::Level::Log:
			{
				constexpr std::wstring_view str = L"Log";
				return std::copy_n(str.data(), str.size(), format_context.out());
			}
			default:
				return format_context.out();
			}
		}
	};

	template<Potato::TMP::TypeString type_string>
	struct formatter<Potato::Log::LogCategory<type_string>, char>
	{
		constexpr auto parse(std::basic_format_parse_context<char>& parse_context)
		{
			if (*parse_context.begin() == L'}')
				return parse_context.begin();
			else
				throw "LogLevel do not support any parameter";
		}
		template<typename FormatContext>
		constexpr auto format(Potato::Log::LogCategory<N> const& category, FormatContext& format_context) const
		{
			auto str = category.GetStringView();
			return std::copy_n(str.data(), str.size(), format_context.out());
		}
	};

	template<>
	struct formatter<Potato::Log::FormatedSystemTime, char>
	{
		constexpr auto parse(std::basic_format_parse_context<char>& parse_context)
		{
			return parse_context.begin();
		}

		template<typename FormatContext>
		constexpr auto format(Potato::Log::FormatedSystemTime time, FormatContext& format_context) const
		{
			auto second_now = std::chrono::floor<std::chrono::seconds>(time.current_time);

			auto zoned_time = std::chrono::zoned_time{
				std::chrono::current_zone(),
				second_now
			};

			auto mil_second = std::chrono::floor<std::chrono::milliseconds>(time.current_time - second_now);

			return std::format_to(
				format_context.out(),
				"{:%m.%d-%H:%M:%S}.{:0>3}",
				zoned_time,
				mil_second.count()
			);
		}
	};
}



export namespace Potato::Log
{

	struct LogPrinter
	{
		struct Wrapper
		{
			void AddRef(LogPrinter const* ptr) { ptr->AddLogPrinterRef(); }
			void SubRef(LogPrinter const* ptr) { ptr->SubLogPrinterRef(); }
		};
		virtual std::pmr::memory_resource* GetMemoryResource(Level level) { return std::pmr::get_default_resource(); };
		virtual void Print(Level level, std::string_view output) = 0;
		virtual void Done(std::pmr::memory_resource* resource) {}
		using Ptr = Pointer::IntrusivePtr<LogPrinter, Wrapper>;
	protected:
		virtual void AddLogPrinterRef() const = 0;
		virtual void SubLogPrinterRef() const = 0;
	};

	LogPrinter::Ptr GetLogPrinter();

	struct DefaultLogFormatter;

	template<LogCategory category>
	struct LogCategoryProperty
	{
		static bool IsLogEnable(Level level) { return true; }
	};

	template<LogCategory category>
	struct LogCategoryFormatter
	{
		template<typename OutputIterator, typename ...Parameters>
		OutputIterator operator()(OutputIterator iterator, Level level, std::basic_format_string<char, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
		{

			FormatedSystemTime time;

			iterator = std::format_to(
				std::move(iterator),
				"[{}]{}<{}>",
				time, category, level
			);

			iterator = std::format_to(
				std::move(iterator),
				pattern,
				std::forward<Parameters>(parameters)...
			);

			*iterator++ = '\n';

			return iterator;
		}
	};

	template<LogCategory category, typename ...Parameters>
	constexpr void Log(Level level, std::basic_format_string<char, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
	{
		if (LogCategoryProperty<category>::IsLogEnable(level))
		{
			auto printer = GetLogPrinter();
			if (printer)
			{
				auto resource = printer->GetMemoryResource(level);
				{
					std::pmr::string cache_log{ resource };
					cache_log.reserve(1024);
					LogCategoryFormatter<category>{}(std::back_insert_iterator{ cache_log }, level, pattern, std::forward<Parameters>(parameters)...);
					printer->Print(level, cache_log);
				}
				printer->Done(resource);
			}
		}
	}
}

