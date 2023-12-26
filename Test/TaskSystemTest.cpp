import std;
import PotatoTaskSystem;

using namespace Potato::Task;


int main()
{
	{
		auto Ptr = TaskContext::Create();
		
		std::size_t Count = 0;
		auto Lambda = Task::CreatLambdaTask([&Count](Potato::Task::ExecuteStatus Status, Potato::Task::Task::Ptr This){
			std::println("Count :{0} {1}", Count, static_cast<std::size_t>(Status));
			Count++;
			if(Count <= 20)
				Status.Context.CommitDelayTask(This, std::chrono::system_clock::now() + std::chrono::milliseconds{50});
		});

		auto Lambda2 = Task::CreatLambdaTask([&Count](Potato::Task::ExecuteStatus Status, Potato::Task::Task::Ptr This) {
			std::println("Count :{0} {1}", Count, static_cast<std::size_t>(Status));
			Count++;
			if (Count <= 20)
				Status.Context.CommitDelayTask(This, std::chrono::system_clock::now() + std::chrono::milliseconds{ 50 });
			});

		Ptr->CommitTask(Lambda);
		Ptr->CommitTask(Lambda2);
		Ptr->FireThreads();
		Ptr->WaitTask();
	}

	volatile int i = 0;
}