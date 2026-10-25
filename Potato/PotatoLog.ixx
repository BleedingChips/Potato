export module PotatoLog;
import std;
import PotatoTMP;
import PotatoEncode;
import PotatoPointer;
import PotatoFormat;
import PotatoMisc;

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

	/*
	template<TMP::TypeString category>
	struct LogCategory
	{
		static constexpr bool need_time_catgeory_level_prefix = false;
	};

	template<typename Category>
	concept IsAcceableLogCatgory = requires(Category category)
	{
		{ Category::need_time_catgeory_level_prefix } -> std::same_as<bool>;
		std::formatter<decltype(Category::category_name), char>{};
	};
	*/
}

export namespace std
{

	template<>
	struct formatter<Potato::Log::LogLevel, wchar_t>
	{
		constexpr auto parse(std::basic_format_parse_context<wchar_t>& parse_context)
		{
			if (*parse_context.begin() == L'}')
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
			if (*parse_context.begin() == L'}')
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
		virtual std::pmr::memory_resource* GetSynchronizedPoolResource() { return std::pmr::get_default_resource(); };
		virtual void Print(FormatedSystemTime time, LogLevel level, std::pmr::wstring const& message) = 0;
		virtual void ExceptionRecover(FormatedSystemTime time, LogLevel level) { }
		virtual std::wstring_view EndLineString() const { return L"\n"; };
		using Ptr = Pointer::IntrusivePtr<LogPrinter, Wrapper>;
	protected:
		virtual void AddLogPrinterRef() const = 0;
		virtual void SubLogPrinterRef() const = 0;
	};

	//std::true_type

	struct DefaultLogFormatter
	{
		template<typename OutIterator, typename ...Parameters>
		static OutIterator LogFormat(OutIterator iterator, FormatedSystemTime time, std::basic_format_string<wchar_t, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
		{
			return std::format_to(
				std::move(iterator),
				pattern,
				std::forward<Parameters>(parameters)...
			);
		}
		static constexpr LogLevel value = LogLevel::Log;
	};

	template<TMP::TypeString pattern>
	struct AcceptableLogPattern
	{
		static constexpr decltype(auto) GetPattern() { return pattern.GetStringView(); }
	};

	template<TMP::TypeString log_name, LogLevel level>
	struct NamedPropertyLogFormatter
	{

		using LogNameT = AcceptableLogPattern<TMP::TypeStringEncoder<log_name, wchar_t>::EncodeTo()>;

		template<typename OutIterator, typename ...Parameters>
		static OutIterator LogFormat(OutIterator iterator, FormatedSystemTime time, std::basic_format_string<wchar_t, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
		{

			iterator = std::format_to(
				std::move(iterator),
				L"[{}]{}<{}>:",
				time,
				LogNameT::GetPattern(),
				level
			);

			return std::format_to(
				std::move(iterator),
				pattern,
				std::forward<Parameters>(parameters)...
			);

		}
		static constexpr LogLevel value = level;
	};

	LogPrinter::Ptr GetSTDOutPrinter();

	


	template<typename LogType, TMP::TypeString pattern, typename ...Parameters>
	constexpr void LogTo(LogPrinter& printer, Parameters&& ...parameters)
	{
		FormatedSystemTime system_time;
		std::array<wchar_t, 512> cachue_buffer;

		std::pmr::monotonic_buffer_resource buffer_resource{
			static_cast<void*>(cachue_buffer.data()),
			cachue_buffer.size() * sizeof(decltype(cachue_buffer)::value_type),
			printer.GetSynchronizedPoolResource()
		};

		std::pmr::wstring message{&buffer_resource };

		using PatternT = AcceptableLogPattern<TMP::TypeStringEncoder<pattern, wchar_t>::EncodeTo()>;

		//consteval auto char_pattern = TMP::TypeStringEncoder<pattern, char>::EncodeTo();

		auto str = PatternT::GetPattern();

		try {
			LogType::LogFormat(std::back_insert_iterator{message}, system_time, PatternT::GetPattern(), AddLogStringWrapper(std::forward<Parameters>(parameters))...);
			std::format_to(
				std::back_insert_iterator{ message },
				L"{}",
				printer.EndLineString()
			);
			printer.Print(system_time, LogType::value, message);
		}
		catch (...)
		{
			printer.ExceptionRecover(system_time, LogType::value);
			throw ;
		}
	}

	template<TMP::TypeString pattern, typename ...Parameters>
	constexpr void LogTo(LogPrinter& printer, Parameters&& ...parameters)
	{
		return LogTo<DefaultLogFormatter, pattern>(printer, std::forward<Parameters>(parameters)...);
	}

	template<TMP::TypeString pattern, typename ...Parameters>
	constexpr void Log(Parameters&& ...parameters)
	{
		return LogTo<pattern>(*GetSTDOutPrinter(), std::forward<Parameters>(parameters)...);
	}


	template<TMP::TypeString log_name, LogLevel level, TMP::TypeString pattern, typename ...Parameters>
	constexpr void LogTo(LogPrinter& printer, Parameters&& ...parameters)
	{
		return LogTo<NamedPropertyLogFormatter<log_name, level>, pattern>(printer, std::forward<Parameters>(parameters)...);
	}

	template<TMP::TypeString log_name, LogLevel level, TMP::TypeString pattern, typename ...Parameters>
	constexpr void Log(Parameters&& ...parameters)
	{
		return LogTo<log_name, level, pattern>(*GetSTDOutPrinter(), std::forward<Parameters>(parameters)...);
	}


	/*
	template<typename LogType, typename ...Parameters>
	constexpr void LogTo(LogPrinter& printer, std::basic_format_string<char, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
	{
		return LogTo<LogType>(printer, Log::LogLevel::Log, pattern, std::forward<Parameters>(parameters)...);
	}

	template<typename LogType, typename ...Parameters>
	constexpr void Log(LogLevel level, std::basic_format_string<char, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
	{
		return LogTo<LogType>(*GetSTDOutPrinter(), level, pattern, std::forward<Parameters>(parameters)...);
	}

	template<typename LogType, typename ...Parameters>
	constexpr void Log(std::basic_format_string<char, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
	{
		return LogTo<LogType>(*GetSTDOutPrinter(), pattern, std::forward<Parameters>(parameters)...);
	}
	*/


	/*
	LogTo<Category>(Log::LogLevel::Log, "")([]() {return });
	

	template<IsAcceableLogCatgory Category, LogLevel level>
	struct LogCategoryFormatter
	{
		template<typename OutputIterator, typename ...Parameters>
		OutputIterator operator()(OutputIterator iterator, std::basic_format_string<char, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
		{

			if constexpr (Category::need_time_catgeory_level_prefix)
			{
				FormatedSystemTime time;
				iterator = std::format_to(
					std::move(iterator),
					"[{}]{}<{}>:",
					time, category, level
				);
			}
			
			return std::format_to(
				std::move(iterator),
				pattern,
				std::forward<Parameters>(parameters)...
			);
		}
	};

	template<TMP::TypeString category, LogLevel level, TMP::TypeString formatter, typename ...Parameters>
	constexpr void LogTo(LogPrinter& printer, std::basic_format_string<char, std::type_identity_t<Parameters>...> const& pattern, Parameters&& ...parameters)
	{

		static constexpr auto name = TMP::TypeStringEncoder<category, char>::EncodeTo();

		LogLine line;

		auto iterator = printer.GetUniqueOutIterator();

		LogCategoryFormatter<category, level>{}(
			iterator,
			pattern,
			AddLogStringWrapper(std::forward<Parameters>(parameters))...
			);

		auto end_line = printer.EndLineString();

		for (auto ite : end_line)
		{
			*(iterator++) = ite;
		}

		line.category = name.GetStringView();
		line.level = level;
		line.log_message = message;
		printer.Print(line);
	}

	template<TMP::TypeString category, LogLevel level, TMP::TypeString formatter, typename ...Parameters>
	constexpr void Log(Parameters&& ...parameters)
	{
		if (LogCategoryProperty<category>::IsLogEnable(level))
		{
			auto printer = GetLogPrinter();
			if (printer)
			{
				LogTo<category, level, formatter, Parameters...>(*printer, std::forward<Parameters>(parameters)...);
			}
		}
	}
	*/
}

