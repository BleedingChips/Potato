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
		void Print(FormatedSystemTime time, LogLevel level, std::pmr::wstring const& message) override
		{
			std::wcout << message;
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

	LogPrinter::Ptr GetSTDOutPrinter()
	{
		return &printer;
	}
}