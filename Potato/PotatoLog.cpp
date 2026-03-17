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
		void Print(LogLine const& log_line) override
		{
			std::wcout << log_line.log_message;
		}
		virtual void AddLogPrinterRef() const {}
		virtual void SubLogPrinterRef() const {}
		std::wstring_view EndLineString() const { return L"\n"; };
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
}