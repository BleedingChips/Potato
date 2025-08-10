
module PotatoLog;




namespace Potato::Log
{

	std::pmr::memory_resource* LogMemoryResource() { return std::pmr::get_default_resource(); }

	struct TerminalLogPrinter : public LogPrinter
	{

		void Print(std::wstring_view print)
		{
			return;
			std::wcout << print;
		}

		TerminalLogPrinter()
		{
			std::ios::sync_with_stdio(false);
			constexpr char locale_name[] = "";
			std::locale::global(std::locale(locale_name));
			std::wcin.imbue(std::locale());
			std::wcout.imbue(std::locale());
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
}