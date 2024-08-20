import std;
import PotatoTaskSystem;
import PotatoTaskFlow;
import PotatoFormat;
import PotatoEncode;

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

struct DefaultTaskFlow : TaskFlow
{
	void AddTaskFlowRef() const override {}
	void SubTaskFlowRef() const override {}
	void TaskFlowExecuteBegin(TaskFlowContext& context) override
	{
		auto wstr = *Potato::Encode::StrEncoder<char8_t, wchar_t>::EncodeToString(std::u8string_view{context.node_property.display_name});
		auto sstr = *Potato::Encode::StrEncoder<wchar_t, char>::EncodeToString(std::wstring_view{wstr});
		{
			std::lock_guard lg(print_mutex);
			std::println("Task Flow <{0}> Begin - {1} ----------", sstr, std::this_thread::get_id());
		}
	}

	void TaskFlowExecuteEnd(TaskFlowContext& context) override
	{
		auto wstr = *Potato::Encode::StrEncoder<char8_t, wchar_t>::EncodeToString(std::u8string_view{context.node_property.display_name});
		auto sstr = *Potato::Encode::StrEncoder<wchar_t, char>::EncodeToString(std::wstring_view{wstr});
		{
			std::lock_guard lg(print_mutex);
			std::println("Task Flow <{0}> End - {1} ----------", sstr, std::this_thread::get_id());
		}
	}
};

int main()
{

	{
		DefaultTaskFlow tf2;
		DefaultTaskFlow tf;
		

		TaskContext context;

		std::size_t Count = 0;
		auto A1 = TaskFlow::CreateLambdaTask([](Potato::Task::TaskFlowContext& status){
			Print(status.node_property.display_name, std::this_thread::get_id());
		});

		auto lambda = [](Potato::Task::TaskFlowContext& status){
			Print(status.node_property.display_name, std::this_thread::get_id());
		};

		auto lambda2 = [&](Potato::Task::TaskFlowContext& context)
		{
			auto nodex = TaskFlow::CreateLambdaTask(lambda);
			context.flow->AddTemporaryNode(*nodex, {});
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

		bool l12 = tf.AddDirectEdge(*a1, *a2);
		bool l23 = tf.AddDirectEdge(*a2, *a3);
		bool l34 = tf.AddDirectEdge(*a3, *a4);
		bool l41 = tf.AddDirectEdge(*a4, *a1);
		bool m35 = tf.AddMutexEdge(*a3, *a5);
		
		bool r41 = tf.RemoveDirectEdge(*a2, *a3);
		bool l41_2 = tf.AddDirectEdge(*a4, *a1);
		bool l61 = tf.AddDirectEdge(tf2, *a1);
		bool l46 = tf.AddDirectEdge(*a4, tf2);

		auto l_12 = tf2.AddDirectEdge(*a21, *a22);

		tf.Update();

		context.AddGroupThread({}, 5);

		tf.Commited(context, {u8"first Task Flow"});

		context.ProcessTaskUntillNoExitsTask({});

		//auto pro = tf.CreateProcessor();

		tf.Update();

		tf.Commited(context, {u8"second Task Flow"});

		context.ProcessTaskUntillNoExitsTask({});
	}

	volatile int i = 0;
}