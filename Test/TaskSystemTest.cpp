import std;
import PotatoTaskSystem;

using namespace Potato::Task;


int main()
{
	{
		auto Ptr = TaskContext::Create();
		Ptr->FireThreads(1);
		std::size_t Count = 0;
		auto Lambda = Task::CreatLambdaTask([&Count](Potato::Task::ExecuteStatus Status, Potato::Task::TaskContext& Context, Potato::Task::Task::Ptr This){
			std::println("Count :{0} {1}", Count, static_cast<std::size_t>(Status));
			Count++;
			if(Count <= 20)
				Context.CommitDelayTask(This, std::chrono::system_clock::now() + std::chrono::milliseconds{50});
		});
		Ptr->CommitTask(Lambda);
	}

	volatile int i = 0;
}