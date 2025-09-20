module;
#ifdef _WIN32
#include "Windows.h"
#endif


module PotatoLog;

namespace Potato::Log
{
	LogPrinter::Ptr GetLogPrinter()
	{
		return {};
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