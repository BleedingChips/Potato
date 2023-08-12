import PotatoTaskSystem;

using namespace Potato::Task;


int main()
{
	{
		auto Ptr = TaskContext::Create();
		std::size_t Count = 0;
		auto Lambda = Task::CreateLambda({}, [&Count](ExecuteInfo Info){
			std::println("Count :{0} {1}", Count, static_cast<std::size_t>(Info.Status));
			Count++;
			Info.Context.CommitDelayTask(Info.Self, std::chrono::system_clock::now() + std::chrono::milliseconds{ 100 });
		});
		Ptr->CommitTask(Lambda);
		Ptr->FireThreads();
		std::this_thread::sleep_for(std::chrono::seconds{10});
		Ptr->CloseThreads();
		Ptr->ExecuteAndRemoveAllTask();
	}
}