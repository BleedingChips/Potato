

import PotatoLog;
import std;

using namespace Potato::Log;


int main()
{


	auto L = std::format(L"{} {} {}", Potato::Log::Level::Log, 122345, L"ÎÒÈ¥ÄãµÄ");

	//std::wcout << L << std::endl;
	Log<L"Fuck">(Level::Log, L"1234 {}", 1);

	volatile int i = 0;
}