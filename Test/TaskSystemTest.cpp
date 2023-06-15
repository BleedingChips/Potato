import Potato.TaskSystem;

using namespace Potato::Task;


int main()
{
	{

		auto Ptr = TaskSystem::Create();
		std::this_thread::sleep_for(std::chrono::seconds{10});
	}
}