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

	template<TMP::TypeString type_string>
	struct LogCategory
	{

		using K = TMP::TypeStringEncoder<type_string>;


		static constexpr auto name = K::EncodeTo();
	};

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
		std::wstring_view category;
		LogLevel level;
		std::wstring_view log_message;
	};

	template<Potato::TMP::TypeString type_string>
	struct LogFormatter
	{
		static constexpr auto pattern = []() { 
			return Potato::TMP::TypeStringEncoder<decltype(type_string)::Type> ::EncodeTo();
			}();
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
			case Potato::Log::LogLevel::VeryVerbose:
			{
				constexpr std::wstring_view str = L"VeryVerbose";
				return std::copy_n(str.data(), str.size(), format_context.out());
			}
			case Potato::Log::LogLevel::Verbose:
			{
				constexpr std::wstring_view str = L"Verbose";
				return std::copy_n(str.data(), str.size(), format_context.out());
			}
			case Potato::Log::LogLevel::Log:
			{
				constexpr std::wstring_view str = L"Log";
				return std::copy_n(str.data(), str.size(), format_context.out());
			}
			case Potato::Log::LogLevel::Display:
			{
				constexpr std::wstring_view str = L"Display";
				return std::copy_n(str.data(), str.size(), format_context.out());
			}
			case Potato::Log::LogLevel::Warning:
			{
				constexpr std::wstring_view str = L"Warning";
				return std::copy_n(str.data(), str.size(), format_context.out());
			}
			case Potato::Log::LogLevel::Error:
			{
				constexpr std::wstring_view str = L"Error";
				return std::copy_n(str.data(), str.size(), format_context.out());
			}
			default:
				return format_context.out();
			}
		}
	};

	template<Potato::TMP::TypeString type_string>
	struct formatter<Potato::Log::LogCategory<type_string>, wchar_t>
	{
		constexpr auto parse(std::basic_format_parse_context<wchar_t>& parse_context)
		{
			if (*parse_context.begin() == L'}')
				return parse_context.begin();
			else
				throw "LogLevel do not support any parameter";
		}
		template<typename FormatContext>
		constexpr auto format(Potato::Log::LogCategory<type_string> category, FormatContext& format_context) const
		{
			auto str = category.name.GetStringView();
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
		virtual void Print(LogLine const& log_lone) = 0;
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
		static bool IsLogEnable(LogLevel level) { return true; }
	};

	template<LogCategory category, LogLevel level>
	struct LogCategoryFormatter
	{
		template<typename OutputIterator, typename ...Parameters>
		OutputIterator operator()(OutputIterator iterator, std::basic_format_string<wchar_t, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
		{

			FormatedSystemTime time;

			iterator = std::format_to(
				std::move(iterator),
				L"[{}]{}<{}>",
				time, category, level
			);

			iterator = std::format_to(
				std::move(iterator),
				pattern,
				std::forward<Parameters>(parameters)...
			);

			return iterator;
		}
	};

	template<LogCategory category, LogLevel level, LogFormatter formatter, typename ...Parameters>
	constexpr void Log(Parameters&& ...parameters)
	{
		if (LogCategoryProperty<category>::IsLogEnable(level))
		{
			LogLine line;
			auto printer = GetLogPrinter();
			if (printer)
			{
				std::wstring message;
				LogCategoryFormatter<category, level>{}(
					std::back_inserter(message),
					formatter.patten.GetStringView(),
					std::forward<Parameters>(parameters)...
					);

				line.category = category.name.GetStringView();
				line.level = level;
				line.log_message = message;
				printer->Print(line);
			}
		}
	}
}

