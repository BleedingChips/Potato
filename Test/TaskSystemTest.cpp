import PotatoTaskSystem;

using namespace Potato::Task;


int main()
{
	{
		auto Ptr = Context::Create();
		std::this_thread::sleep_for(std::chrono::seconds{10});
	}
}