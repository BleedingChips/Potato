

import PotatoLog;
import std;

using namespace Potato::Log;


int main()
{

	Log<L"Fuck", Level::Log, L"babababaGo {}">(1);

	volatile int i = 0;
}