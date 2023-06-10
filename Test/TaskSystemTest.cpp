import Potato.TaskSystem;
using namespace Potato::Task;


int main()
{
	{
		TaskSystem TS{};
		std::this_thread::sleep_for(std::chrono::seconds{10});
	}
}