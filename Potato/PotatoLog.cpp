module PotatoLog;

namespace Potato::Log
{
	void TerminalOutput::operator()(std::wstring_view output)
	{
		static std::mutex print_mutex;
		std::wcout << output << std::endl;
	}
}