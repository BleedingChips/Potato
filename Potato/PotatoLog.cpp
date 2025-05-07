module PotatoLog;

namespace Potato::Log
{

	std::pmr::memory_resource* LogMemoryResource() { return std::pmr::get_default_resource(); }

	struct TerminalLogPrinter : public LogPrinter
	{
		std::mutex output_mutex;

		void Print(std::wstring_view print)
		{
			std::array<char, 2048> output_buffer;

			auto info = Encode::StrEncoder<wchar_t, char>::EncodeUnSafe(
				{ print.data(), print.size() },
				output_buffer
			);

			std::cout << std::string_view{ output_buffer.data(), info.TargetSpace };
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