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
			if(Info)
			{
				Info.Context.Commit(Info.Self);
				std::this_thread::sleep_for(std::chrono::microseconds{ 10 });
			}
			volatile int o = 0;
		});
		Ptr->Commit(Lambda);
		Ptr->FireThreads();
		std::this_thread::sleep_for(std::chrono::seconds{10});
		Ptr->CloseThreads();
		Ptr->ExecuteAndRemoveAllTask();
	}
}