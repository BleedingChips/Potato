module PotatoLog;

namespace Potato::Log
{
	void DirectPrintWCout(std::wstring_view log)
	{
		std::wcout << log << std::endl;
	}
}