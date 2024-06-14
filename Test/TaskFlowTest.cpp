import std;
import PotatoTaskSystem;
import PotatoTaskFlow;

using namespace Potato::Task;

std::mutex print_mutex;

void Print(std::string_view str)
{
	{
		std::lock_guard lg(print_mutex);
		std::println("{0} Begin", str);
	}
	
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });

	{
		std::lock_guard lg(print_mutex);
		std::println("{0} End", str);
	}
}

struct DefaultTaskFlow : TaskFlow
{
	void AddTaskFlowRef() const override {}
	void SubTaskFlowRef() const override {}
};

int main()
{

	{

		TaskContext context;

		std::size_t Count = 0;
		auto A1 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowContext& status){
			Print("A1");
			
		});

		auto A2 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowContext& status) {
			Print("A2");
		});

		auto A3 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowContext& status) {
			Print("A3");
		});

		auto A4 = TaskFlowNode::CreateLambda([](Potato::Task::TaskFlowContext& status) {
			Print("A4");
			});

		DefaultTaskFlow tf;

		//auto G1 = TaskFlow::CreateDefaultTaskFlow();

		auto a1 = tf.AddNode(A1, {u8"A1"});
		auto a2 = tf.AddNode(A2, {u8"A2"});
		auto a3 = tf.AddNode(A3, {u8"A3"});
		auto a4 = tf.AddNode(A4, {u8"A4"});
		auto a5 = tf.AddNode(A4, {u8"A5"});

		bool l12 = tf.AddDirectEdge(*a1, *a2);
		bool l23 = tf.AddDirectEdge(*a2, *a3);
		bool l34 = tf.AddDirectEdge(*a3, *a4);
		bool l41 = tf.AddDirectEdge(*a4, *a1);
		bool r41 = tf.RemoveDirectEdge(*a2, *a3);
		bool l41_2 = tf.AddDirectEdge(*a4, *a1);
		volatile int i = 0;

		/*
		tf.AddDirectEdges(A1, A2);
		//tf.AddDirectEdges(A2, A1);
		tf.AddDirectEdges(A1, A2);
		tf.AddDirectEdges(A1, A3);
		tf.AddDirectEdges(A2, A3);
		tf.AddMutexEdges(A1, A4);

		std::pmr::vector<TaskFlow::ErrorNode> Error;

		
		tf.TryUpdate(&Error);
		tf.Remove(A4);


		tf.TryUpdate(&Error);

		tf.Commit(context);
		context.AddGroupThread({}, TaskContext::GetSuggestThreadCount());
		context.ProcessTaskUntillNoExitsTask({});
		//context.ProcessTaskUntillNoExitsTask({});
		*/
		/*

		auto G2 = TaskFlow::CreateDefaultTaskFlow();


		{
			TaskFlowGraphic gra;

			auto A1N = *gra.AddNode(A1);
			auto A2N = *gra.AddNode(A2);
			auto G2N = *gra.AddNode(G2);

			gra.AddDirectedEdge(A1N, A2N);

			gra.AddDirectedEdge(A1N, G2N);

			G1->ResetGraphic(gra);
		}


		{
			TaskFlowGraphic gra;

			auto A3N = *gra.AddNode(A3);
			auto A4N = *gra.AddNode(A4);

			gra.AddDirectedEdge(A3N, A4N);

			G2->ResetGraphic(gra);
		}
		*/
		/*
		auto A1N = *G1->AddStaticNode(A1);
		auto A2N = *G1->AddStaticNode(A2);



		auto G2 = TaskFlow::CreateDefaultTaskFlow();


		TaskFlowGraphic gre;


		auto A3N = *G2->AddStaticNode(A3);
		auto A4N = *G2->AddStaticNode(A4);

		auto G2N = *G1->AddStaticNode(G2);
		*/


		TaskProperty tp;

		/*
		
		*/
	}

	volatile int i = 0;
}