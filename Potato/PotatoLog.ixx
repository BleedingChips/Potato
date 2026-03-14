export module PotatoLog;
import std;
import PotatoTMP;
import PotatoEncode;
import PotatoPointer;
import PotatoFormat;

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

	template<typename CharT, typename CharTraits = std::char_traits<CharT>>
	struct LogStringViewWrapper
	{
		std::basic_string_view<CharT, CharTraits> string;
	};

	template<typename CharT, typename CharTraits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
	struct LogStringWrapper
	{
		std::basic_string<CharT, CharTraits, Allocator>& string;
	};

	template<typename CharT>
	struct LogStringWrapperDetect
	{
		static constexpr bool require_wrapper = false;
	};

	template<typename CharT, typename CharTraits>
	struct LogStringWrapperDetect<std::basic_string_view<CharT, CharTraits>>
	{
		static constexpr bool require_wrapper = true;
		using Type = LogStringViewWrapper<CharT, CharTraits>;
	};

	template<typename CharT, std::size_t N>
	struct LogStringWrapperDetect<CharT[N]>
	{
		static constexpr bool require_wrapper = true;
		using Type = LogStringViewWrapper<CharT>;
	};

	template<typename CharT, typename CharTraits, typename Allocator>
	struct LogStringWrapperDetect<std::basic_string<CharT, CharTraits, Allocator>>
	{
		static constexpr bool require_wrapper = true;
		using Type = LogStringWrapper<CharT, CharTraits, Allocator>;
	};

	template<typename Input>
	decltype(auto) AddLogStringWrapper(Input&& input)
	{
		using Type = LogStringWrapperDetect<std::remove_cvref_t<Input>>;

		if constexpr (Type::require_wrapper)
		{
			return typename Type::Type{ input };
		}
		else {
			return std::forward<Input>(input);
		}
	}
}

export namespace std
{
	template<>
	struct formatter<Potato::Log::LogLevel, wchar_t>
	{
		constexpr auto parse(std::basic_format_parse_context<wchar_t>& parse_context)
		{
			if(*parse_context.begin() == '}')
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

	template<>
	struct formatter<Potato::Log::FormatedSystemTime, wchar_t>
	{
		constexpr auto parse(std::basic_format_parse_context<wchar_t>& parse_context)
		{
			if (*parse_context.begin() == static_cast<wchar_t>('}'))
				return parse_context.begin();
			else
				throw "std::formatter<LogLevel> do not support any parameter";
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
				L"{:%m.%d-%H:%M:%S}.{:0>3}",
				zoned_time,
				mil_second.count()
			);
		}
	};

	template<typename CharT, typename TargetCharT>
	struct formatter<Potato::Log::LogStringWrapper<CharT>, TargetCharT>
	{
		constexpr auto parse(std::basic_format_parse_context<TargetCharT>& parse_context)
		{
			if (*parse_context.begin() == static_cast<TargetCharT>('}'))
				return parse_context.begin();
			else
				throw "std::formatter<Potato::Log::LogStringWrapper> do not support any parameter";
		}
		template<typename FormatContext>
		constexpr auto format(Potato::Log::LogStringWrapper<CharT> string_wrapper, FormatContext& format_context) const
		{
			auto [info, iterator] = Potato::Encode::UnicodeEncoder<CharT, TargetCharT>::EncodeTo(
				string_wrapper.string,
				format_context.out()
			);
			return iterator;
		}
	};

	template<typename CharT, typename TargetCharT>
	struct formatter<Potato::Log::LogStringViewWrapper<CharT>, TargetCharT>
	{
		constexpr auto parse(std::basic_format_parse_context<TargetCharT>& parse_context)
		{
			if (*parse_context.begin() == static_cast<TargetCharT>('}'))
				return parse_context.begin();
			else
				throw "std::formatter<Potato::Log::LogStringViewWrapper> do not support any parameter";
		}
		template<typename FormatContext>
		constexpr auto format(Potato::Log::LogStringViewWrapper<CharT> string_wrapper, FormatContext& format_context) const
		{
			auto [info, iterator] = Potato::Encode::UnicodeEncoder<CharT, TargetCharT>::EncodeTo(
				string_wrapper.string,
				format_context.out()
			);
			return iterator;
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
		virtual std::wstring_view EndLineString() const { return {}; };
		using Ptr = Pointer::IntrusivePtr<LogPrinter, Wrapper>;
	protected:
		virtual void AddLogPrinterRef() const = 0;
		virtual void SubLogPrinterRef() const = 0;
	};

	LogPrinter::Ptr GetLogPrinter();

	struct DefaultLogFormatter;

	template<TMP::TypeString category>
	struct LogCategoryProperty
	{
		static bool IsLogEnable(LogLevel level) { return true; }
	};

	template<TMP::TypeString category, LogLevel level>
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
			return std::format_to(
				std::move(iterator),
				pattern,
				std::forward<Parameters>(parameters)...
			);
		}
	};

	template<TMP::TypeString category, LogLevel level, TMP::TypeString formatter, typename ...Parameters>
	constexpr void Log(Parameters&& ...parameters)
	{
		static constexpr auto formatterw = TMP::TypeStringEncoder<formatter, wchar_t>::EncodeTo();
		static constexpr auto name = TMP::TypeStringEncoder<category, wchar_t>::EncodeTo();

		if (LogCategoryProperty<category>::IsLogEnable(level))
		{
			LogLine line;
			auto printer = GetLogPrinter();
			if (printer)
			{
				std::array<std::byte, sizeof(wchar_t) * 256> log_output;
				std::pmr::monotonic_buffer_resource buffer_resource{ log_output.data(), log_output.size()};
				std::pmr::wstring message(&buffer_resource);
				
				LogCategoryFormatter<category, level>{}(
					std::back_inserter(message),
					formatterw.GetStringView(),
					AddLogStringWrapper(std::forward<Parameters>(parameters))...
					);

				auto end_line = printer->EndLineString();

				std::copy_n(
					end_line.data(),
					end_line.size(),
					std::back_inserter(message)
				);

				line.category = name.GetStringView();
				line.level = level;
				line.log_message = message;
				printer->Print(line);
			}
		}
	}
}

