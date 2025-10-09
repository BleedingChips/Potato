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

	struct FormatedSystemTime
	{
		std::chrono::system_clock::time_point current_time;

		FormatedSystemTime(std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now())
			:current_time(current_time)
		{
		}
	};
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
				constexpr std::string_view str = "Log";
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

