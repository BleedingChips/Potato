import std;
import PotatoTaskGraphic;
import PotatoTask;
import PotatoFormat;
import PotatoEncode;
import PotatoGraph;

using namespace Potato::Task;

std::mutex print_mutex;

void Print(std::u8string_view str, std::thread::id thread_id)
{
	auto wstr = *Potato::Encode::StrEncoder<char8_t, wchar_t>::EncodeToString(std::u8string_view{str});
	auto sstr = *Potato::Encode::StrEncoder<wchar_t, char>::EncodeToString(std::wstring_view{wstr});
	{
		std::lock_guard lg(print_mutex);
		std::println("{0} Begin - {1}", sstr, thread_id);
	}
	
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });

	{
		std::lock_guard lg(print_mutex);
		std::println("{0} End - {1}", sstr, thread_id);
	}
}

struct DefaultTaskFlow : public Potato::TaskGraphic::Flow
{
	void AddTaskGraphicFlowRef() const override {}
	void SubTaskGraphicFlowRef() const override {}

	void TaskFlowExecuteBegin(Potato::Task::ContextWrapper& wrapper) override
	{
		auto wstr = *Potato::Encode::StrEncoder<char8_t, wchar_t>::EncodeToString(std::u8string_view{ wrapper .GetTaskNodeProperty().node_name});
		auto sstr = *Potato::Encode::StrEncoder<wchar_t, char>::EncodeToString(std::wstring_view{wstr});
		{
			std::lock_guard lg(print_mutex);
			std::println("Task Flow <{0}> Begin - {1} ----------", sstr, std::this_thread::get_id());
		}
	}

	void TaskFlowExecuteEnd(Potato::Task::ContextWrapper& wrapper) override
	{
		auto wstr = *Potato::Encode::StrEncoder<char8_t, wchar_t>::EncodeToString(std::u8string_view{ wrapper.GetTaskNodeProperty().node_name });
		auto sstr = *Potato::Encode::StrEncoder<wchar_t, char>::EncodeToString(std::wstring_view{wstr});
		{
			std::lock_guard lg(print_mutex);
			std::println("Task Flow <{0}> End - {1} ----------", sstr, std::this_thread::get_id());
		}
	}
};

int main()
{

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
}