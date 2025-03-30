import std;
import PotatoTask;

using namespace Potato::Task;


int main()
{
	Context context;
	//context.CreateThreads();

	std::size_t count = 0;

	context.Commit(
		[&](Context& context, Node::Parameter& parameter, Node& self)mutable
		{
			std::this_thread::sleep_for(std::chrono::seconds{ 1 });
			std::println(std::cout, "{0}", count);
			if (count < 4)
			{
				context.Commit(
					self,
					parameter
				);
				count += 1;
			}
		}
	);

	context.ExecuteContextThreadUntilNoExistTask(); 

	return 0;
}