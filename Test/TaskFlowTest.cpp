import std;
import PotatoTaskSystem;
import PotatoTaskFlow;

using namespace Potato::Task;

std::mutex print_mutex;

void Print(std::string_view str)
{
	std::lock_guard lg(print_mutex);
	std::println("{0}", str);
}


int main()
{

	{

		TaskContext context;

		std::size_t Count = 0;
		auto A1 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowStatus& status){
			Print("A1");
		});

		auto A2 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowStatus& status) {
			Print("A2");
		});

		auto A3 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowStatus& status) {
			Print("A3");
		});

		auto A4 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowStatus& status) {
			Print("A4");
			});

		auto G1 = TaskFlow::CreateDefaultTaskFlow();


		auto A1N = *G1->AddStaticNode(A1);
		auto A2N = *G1->AddStaticNode(A2);



		auto G2 = TaskFlow::CreateDefaultTaskFlow();


		TaskFlowGraphic gre;


		auto A3N = *G2->AddStaticNode(A3);
		auto A4N = *G2->AddStaticNode(A4);

		auto G2N = *G1->AddStaticNode(G2);


		TaskProperty tp;
		tp.category = Category::GLOBAL_TASK;
		tp.group_id = 1;
		tp.thread_id = std::this_thread::get_id();

		//Ptr->CommitTask(Lambda2);
		context.AddGroupThread({}, TaskContext::GetSuggestThreadCount());
		context.ProcessTaskUntillNoExitsTask({});
	}

	volatile int i = 0;
}