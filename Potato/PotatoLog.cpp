module;
#ifdef _WIN32
#include "Windows.h"
#undef min
#undef max
#endif
#include <locale>

module PotatoLog;

namespace Potato::Log
{
	struct ConsleLogPrinter : public LogPrinter
	{
		std::mutex log_mutex;
		void Print(LogLine const& log_line) override
		{
			std::lock_guard lg(log_mutex);
			std::wcout << log_line.log_message << L"\n" << std::ends;
		}
		virtual void AddLogPrinterRef() const {}
		virtual void SubLogPrinterRef() const {}
		ConsleLogPrinter()
		{
			std::setlocale(LC_ALL, ".UTF8");
			std::ios::sync_with_stdio(false);
		}
		~ConsleLogPrinter()
		{
			std::wcout.flush();
		}
	}printer;



	LogPrinter::Ptr GetLogPrinter()
	{
		return &printer;
	}

	/*
	std::pmr::memory_resource* LogMemoryResource() { return std::pmr::get_default_resource(); }

	struct TerminalLogPrinter : public LogPrinter
	{

		void Print(std::u8string_view print)
		{
			return;
			std::cout << std::string_view(reinterpret_cast<char const*>(print.data()), print.size());
		}

		TerminalLogPrinter()
		{
#ifdef _WIN32

#endif
			std::ios::sync_with_stdio(false);
			constexpr char locale_name[] = "UTF-8";
			std::locale::global(std::locale(locale_name));
			std::cin.imbue(std::locale());
			std::cout.imbue(std::locale());
		}
	protected:


		virtual void AddLogPrinterRef() const {}
		virtual void SubLogPrinterRef() const {}
	};


	LogPrinter::Ptr GetLogPrinter()
	{
		static TerminalLogPrinter pointer;
		return &pointer;
	}

	LogPrinter::Ptr GetLogPrinter();
	*/
}