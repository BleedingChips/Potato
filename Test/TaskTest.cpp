import std;
import PotatoTask;

using namespace Potato::Task;


int main()
{
	Context context;
	context.AddGroupThread(0, Context::GetSuggestThreadCount());

	std::size_t count = 0;
	auto Task = Node::CreateLambdaNode([&](ContextWrapper& wrapper)
	{
			std::this_thread::sleep_for(std::chrono::seconds{ 1 });
		if (count < 4)
		{
			std::println(std::cout, "{0}", count);
			wrapper.Commit(
				wrapper.GetCurrentTaskNode(),
				wrapper.GetTaskNodeProperty()
			);
			count += 1;
		}
	});

	context.Commit(*Task, {});

	context.ExecuteContextThreadUntilNoExsitTask();

	return 0;
}