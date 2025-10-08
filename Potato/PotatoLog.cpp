module;
#ifdef _WIN32
#include "Windows.h"
#undef min
#undef max
#endif


module PotatoLog;

namespace Potato::Log
{


	struct ConsleLogPrinter : public LogPrinter
	{
		void Print(Level level, std::string_view output) override
		{
#ifdef _WIN32
			std::array<wchar_t, 2048> temp_buffer;
			Potato::Encode::StrEncoder<char, wchar_t> encoder;
			auto info = encoder.Encode(output, std::span(temp_buffer));
			temp_buffer[std::min(info.target_space, temp_buffer.size() - 1)] = L'\0';
			std::wcout << temp_buffer.data();
#else
			std::cout << print << std::endl;
#endif
		}
		virtual void AddLogPrinterRef() const {}
		virtual void SubLogPrinterRef() const {}
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