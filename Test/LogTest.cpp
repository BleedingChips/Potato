
#include <Windows.h>
#include <clocale>
import PotatoLog;
import std;

using namespace Potato::Log;



int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	std::setlocale(LC_ALL, ".UTF8");
	std::ios::sync_with_stdio(false);
	bool l = SetConsoleCP(CP_UTF8);
	//constexpr char locale_name[] = "en_US.UTF-8";
	//std::locale::global(std::locale(locale_name));
	//std::cin.imbue(std::locale());
	//std::cout.imbue(std::locale());

	std::u8string_view output = u8"中国人不骗中国人";
	std::string_view output2 = "中国人不骗中国人";
	//std::print(std::cout, "{}", std::string_view{reinterpret_cast<char const*>(output.data()), output.size()});

	std::cout << std::string_view{ reinterpret_cast<char const*>(output2.data()), output2.size() } << std::endl;


	//auto L = std::format("{} {} {}", Potato::Log::Level::Log, 122345, "输出测试");

	//std::wcout << L << std::endl;
	Log<"Fuck">(Level::Log, "1234 {}", 1);

	volatile int i = 0;
}