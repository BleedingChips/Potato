import std;
import PotatoTaskSystem;

using namespace Potato::Task;


int main()
{

	{

		TaskContext context;

		std::size_t Count = 0;
		auto Lambda = Task::CreateLambdaTask([&Count](Potato::Task::TaskContextWrapper& Status, Potato::Task::Task& This){
			std::println("Count :{0} {1} {2}", Count, static_cast<std::size_t>(Status.status), Status.thread_id);
			Count++;
			if(Count <= 20)
				Status.context.CommitDelayTask(&This, std::chrono::milliseconds{50}, Status.task_property);
		});

		auto Lambda2 = Task::CreateLambdaTask([&Count](Potato::Task::TaskContextWrapper& Status, Potato::Task::Task& This) {
			std::println("Count :{0} {1}", Count, static_cast<std::size_t>(Status.status));
			Count++;
			if (Count <= 20)
				Status.context.CommitDelayTask(&This, std::chrono::milliseconds{ 50 }, Status.task_property);
			});

		TaskProperty tp;
		tp.filter.category = Category::GLOBAL_TASK;
		tp.filter.group_id = 1;
		tp.filter.thread_id = std::this_thread::get_id();

		context.CommitTask(Lambda, tp);
		//Ptr->CommitTask(Lambda2);
		context.AddGroupThread({}, TaskContext::GetSuggestThreadCount());
		context.ProcessTaskUntillNoExitsTask({});
	}

	volatile int i = 0;
}