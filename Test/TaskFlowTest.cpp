import std;
import PotatoTaskFlow;
import PotatoTask;
import PotatoFormat;
import PotatoEncode;
import PotatoGraph;

using namespace Potato::Task;
using namespace Potato;

std::mutex print_mutex;

void Print(TaskFlow::Controller& controller, std::thread::id thread_id)
{
	auto wstr = *Potato::Encode::StrEncoder<char8_t, wchar_t>::EncodeToString(std::u8string_view{ controller.GetParameter().node_name});
	auto sstr = *Potato::Encode::StrEncoder<wchar_t, char>::EncodeToString(std::wstring_view{wstr});

	if (controller.GetCategory() != TaskFlow::EncodedFlow::Category::SubFlowEnd)
	{
		std::lock_guard lg(print_mutex);
		std::println("{0} Begin - {1}", sstr, thread_id);
	}

	if (controller.GetCategory() == TaskFlow::EncodedFlow::Category::NormalNode)
		std::this_thread::sleep_for(std::chrono::milliseconds{ 1000 });

	if (controller.GetCategory() != TaskFlow::EncodedFlow::Category::SubFlowBegin)
	{
		std::lock_guard lg(print_mutex);
		std::println("{0} End - {1}", sstr, thread_id);
	}
}




struct TestNode : public TaskFlow::Node
{

	virtual void TaskFlowNodeExecute(Context& context, TaskFlow::Controller& controller)
	{
		Print(controller, std::this_thread::get_id());
	}

protected:

	virtual void AddTaskGraphicNodeRef() const {}
	virtual void SubTaskGraphicNodeRef() const {}
};

TestNode tnode;

int main()
{
	TaskFlow::Flow flow1;
	TaskFlow::Flow flow2;
	TaskFlow::Flow flow4;
	TaskFlow::Flow flow5;

	auto n1_6 = flow1.AddFlowAsNode(flow4, &tnode, { u8"flow4" });

	auto n5_2 = flow5.AddFlowAsNode(flow1, &tnode);

	auto n1_1 = flow1.AddNode(tnode, {u8"n1_1"});
	auto n1_2 = flow1.AddNode(tnode, { u8"n1_2" });
	auto n1_3 = flow1.AddNode(tnode, { u8"n1_3" });
	auto n1_4 = flow1.AddNode(tnode, { u8"n1_4" });

	auto n2_1 = flow2.AddNode(tnode, { u8"n2_1" });
	auto n2_2 = flow2.AddNode(tnode, { u8"n2_2" });
	auto n2_3 = flow2.AddNode(tnode, { u8"n2_3" });
	auto n2_4 = flow2.AddNode(tnode, { u8"n2_4" });

	auto n2_5 = flow2.AddNode([](Context& context, TaskFlow::Controller& controller)
		{
			Print(controller, std::this_thread::get_id());

			auto mp = controller.MarkCurrentAsPause();

			context.Commit([mp=std::move(mp)](Task::Context& context, Node::Parameter par) mutable 
			{
					{
						std::lock_guard lg(print_mutex);
						std::println("pause - {0}", std::this_thread::get_id());
					}

					std::this_thread::sleep_for(std::chrono::milliseconds{ 5000 });

					{
						std::lock_guard lg(print_mutex);
						std::println("pause done - {0}", std::this_thread::get_id());
					}

					mp.Continue(context);
			});

			controller.AddTemplateNode(
				[](Task::Context& context, TaskFlow::Controller& controller) 
				{ 
					{
						std::lock_guard lg(print_mutex);
						std::println("template - {0}", std::this_thread::get_id());
					}

					std::this_thread::sleep_for(std::chrono::milliseconds{ 5000 });

					{
						std::lock_guard lg(print_mutex);
						std::println("template done - {0}", std::this_thread::get_id());
					}
				},
				[](TaskFlow::Sequencer& sequencer) {  
					auto cur = sequencer.GetCurrentParameter();
					auto ms = sequencer.GetCurrentSubFlow();
					
					return true; 
				}
			);

		}, {u8"lambda"});

	flow2.AddDirectEdge(n2_1, n2_2);
	flow2.AddDirectEdge(n2_2, n2_3);
	flow2.AddDirectEdge(n2_3, n2_4);
	flow2.AddDirectEdge(n2_2, n2_4);

	flow2.AddMutexEdge(n2_1, n2_5);

	

	auto n1_5 = flow1.AddFlowAsNode(flow2, &tnode, { u8"flow2" });


	flow1.AddDirectEdge(n1_4, n1_5);
	flow1.AddDirectEdge(n1_1, n1_2);
	flow1.AddDirectEdge(n1_2, n1_3);
	flow1.AddDirectEdge(n1_3, n1_4);

	TaskFlow::Flow flow3;

	auto n3_1 = flow3.AddFlowAsNode(flow1, &tnode, {u8"flow1"});

	auto instance = TaskFlow::Executor::Create();

	instance->UpdateFromFlow(flow3);

	Task::Context context;
	context.CreateThreads(4);

	instance->Commit(context);

	context.ExecuteContextThreadUntilNoExistTask();

	{
		std::lock_guard lg(print_mutex);
		std::println("----------Fuckk----------");
	}
	

	instance->UpdateState();

	instance->Commit(context);

	context.ExecuteContextThreadUntilNoExistTask();

	volatile int o = 0;


	/*
	Potato::Graph::DirectedAcyclicGraphImmediately grap;

	auto a1 = grap.Add();
	auto a2 = grap.Add();
	auto a3 = grap.Add();
	auto a4 = grap.Add();
	auto a5 = grap.Add();

	bool l12 = grap.AddEdge(a1, a2);
	bool l23 = grap.AddEdge(a2, a3);
	bool l34 = grap.AddEdge(a3, a4);
	bool l41 = grap.AddEdge(a4, a1);

	volatile int i = 0;

	{
		DefaultTaskFlow tf2;
		DefaultTaskFlow tf;
		

		Potato::Task::Context context;

		std::size_t Count = 0;
		auto A1 = Potato::TaskGraphic::Node::CreateLambdaNode([](Potato::TaskGraphic::ContextWrapper& wrapper){
			Print(wrapper.GetNodeProperty().node_name, std::this_thread::get_id());
		});

		auto lambda = [](Potato::TaskGraphic::ContextWrapper& wrapper){
			Print(wrapper.GetNodeProperty().node_name, std::this_thread::get_id());
		};

		auto lambda2 = [&](Potato::TaskGraphic::ContextWrapper& wrapper)
		{
			Print(wrapper.GetNodeProperty().node_name, std::this_thread::get_id());
			auto new_node = Potato::TaskGraphic::Node::CreateLambdaNode([](Potato::TaskGraphic::ContextWrapper& wrapper)
			{
				Print(wrapper.GetNodeProperty().node_name, std::this_thread::get_id());
			});
			wrapper.PauseAndPause(*new_node, { u8"pause" });
		};

		

		auto a1 = tf.AddLambda(lambda, {u8"A1"});
		auto a2 = tf.AddLambda(lambda, {u8"A2"});
		auto a3 = tf.AddLambda(lambda, {u8"A3"});
		auto a4 = tf.AddLambda(lambda, {u8"A4"});
		auto a5 = tf.AddLambda(lambda, {u8"A5"});
		auto a6 = tf.AddNode(tf2, {u8"SubTask"});
		auto a7 = tf.AddLambda(lambda2, {u8"temporary"});
		
		auto a21 = tf2.AddLambda(lambda, {u8"SubTask A1"});
		auto a22 = tf2.AddLambda(lambda, {u8"SubTask A2"});

		bool l12 = tf.AddDirectEdge(a1, a2);
		bool l23 = tf.AddDirectEdge(a2, a3);
		bool l34 = tf.AddDirectEdge(a3, a4);
		bool l41 = tf.AddDirectEdge(a4, a1);
		bool m35 = tf.AddMutexEdge(a3, a5);

		std::array<Potato::Graph::GraphEdge, 100> temp_edge;
		auto p = tf.AcyclicEdgeCheck(temp_edge);

		
		bool r41 = tf.RemoveDirectEdge(a2, a3);

		auto p2 = tf.AcyclicEdgeCheck(temp_edge);
		bool l41_2 = tf.AddDirectEdge(a4, a1);
		bool l61 = tf.AddDirectEdge(a6, a1);
		bool l46 = tf.AddDirectEdge(a4, a6);

		auto l_12 = tf2.AddDirectEdge(a21, a22);

		std::array<Potato::Graph::GraphEdge, 10> error;

		tf.AcyclicEdgeCheck(std::span(error));

		//tf.Update();

		context.AddGroupThread({}, 5);

		tf.Commited(context, {u8"first Task Flow"});

		context.ExecuteContextThreadUntilNoExistTask();

		//auto pro = tf.CreateProcessor();

		//tf.Update();
		tf.AcyclicEdgeCheck(std::span(error));

		tf.Commited(context, {u8"second Task Flow"});

		context.ExecuteContextThreadUntilNoExistTask({});
	}

	volatile int i2 = 0;
	*/
}