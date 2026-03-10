
#include <Windows.h>
#include <clocale>
import PotatoLog;
import std;

using namespace Potato::Log;



int main()
{

	

	//constexpr char locale_name[] = "en_US.UTF-8";
	//std::locale::global(std::locale(locale_name));
	//std::cin.imbue(std::locale());
	//std::cout.imbue(std::locale());

	std::u8string_view output = u8"中国人不骗中国人";
	std::string_view output2 = "中国人不骗中国人";
	//std::print(std::cout, "{}", std::string_view{reinterpret_cast<char const*>(output.data()), output.size()});

	std::u8string_view out3 = {reinterpret_cast<char8_t const*>(output2.data()), output2.size()};

	//std::cout << std::string_view{ reinterpret_cast<char const*>(output2.data()), output2.size() } << std::endl;


	//auto L = std::format("{} {} {}", Potato::Log::Level::Log, 122345, "输出测试");

	//std::wcout << L << std::endl;
	std::u8string_view ptr = u8"中国人好";

	Potato::Log::Log<u8"Fuck", LogLevel::Log, u8"你好中国人 {} {} {}">(1, std::this_thread::get_id(), ptr);
	Potato::Log::Log<u8"Fuck", LogLevel::Log, u8"你好中国人233 {} {} {}">(1, std::this_thread::get_id(), u8"sdasdasd");
	Potato::Log::Log<u8"Fuck", LogLevel::Log, u8"你好中国人233 {} {} {}">(1, std::this_thread::get_id(), std::basic_string<char8_t>{u8"sdasdasd"});

	volatile int i = 0;
}