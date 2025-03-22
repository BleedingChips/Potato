module;

#include <cassert>

export module PotatoTask;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;
import PotatoTMP;


export namespace Potato::Task
{

	using TimeT = std::chrono::steady_clock;

	export struct Context;
	export struct Node;

	enum class Status : std::size_t
	{
		Normal,
		Terminal,
	};

	enum class Priority : std::size_t
	{
		High = 0,
		Normal = 1,
		Low = 2,

		Max
	};

	struct CustomData
	{
		std::size_t data1 = 0;
		std::size_t data2 = 0;
	};

	export struct Node
	{

		struct Wrapper
		{
			void AddRef(Node const* ptr) { ptr->AddTaskNodeRef(); }
			void SubRef(Node const* ptr) { ptr->SubTaskNodeRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<Node, Wrapper>;

		struct Parameter
		{
			std::u8string_view node_name;
			std::size_t acceptable_mask = std::numeric_limits<std::size_t>::max();
			CustomData custom_data;
			std::optional<TimeT::duration> delay_time;
		};

		virtual void TaskExecute(Context& context, Node& self, Parameter& parameter) = 0;
		virtual void TaskTerminal(Node& self, Parameter& parameter) noexcept {};
		virtual ~Node() = default;

	protected:
		
		virtual void AddTaskNodeRef() const = 0;
		virtual void SubTaskNodeRef() const = 0;
	};

	template<typename Type>
	concept AcceptableTaskNode = std::is_invocable_v<Type, Context&, Node&, Node::Parameter&>;

	struct ThreadProperty
	{
		std::size_t acceptable_mask = std::numeric_limits<std::size_t>::max();
	};


	export struct Context
	{
		static std::size_t GetSuggestThreadCount() { return std::thread::hardware_concurrency(); }

		std::optional<std::size_t> CreateThreads(std::size_t thread_count = 1, ThreadProperty thread_property = {});

		struct Config
		{
			std::size_t startup_acceptable_mask = std::numeric_limits<std::size_t>::max();
			std::pmr::memory_resource* resource = std::pmr::get_default_resource();
		};

		Context(Config config = {});
		~Context();

		

		struct ExecuteResult
		{
			std::optional<std::size_t> exist_delay_node;
			std::size_t exist_node;
			bool has_been_execute = false;
		};

		void ExecuteContextThreadOnce(ExecuteResult& result, ThreadProperty thread_property, TimeT::time_point now = TimeT::now());
		void FinishExecuteContext(ExecuteResult& result);

		void ExecuteContextThreadUntilNoExistTask();

		bool Commit(Node& node, Node::Parameter parameter = {});

		template<AcceptableTaskNode FunT>
		static Node::Ptr CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


	protected:

		struct NodeTuple
		{
			Node::Ptr node;
			Node::Parameter parameter;
		};

		struct TimedNodeTuple
		{
			NodeTuple node_tuple;
			TimeT::time_point request_time;
		};

		std::mutex delay_node_sequencer_mutex;
		std::optional<TimeT::time_point> min_time_point;
		std::pmr::vector<TimedNodeTuple> delay_node_sequencer;

		std::mutex node_sequencer_mutex;
		std::pmr::vector<NodeTuple> node_sequencer;
		std::size_t exist_node_count = 0;

		struct ThreadInfo
		{
			std::jthread thread;
			std::thread::id thread_id;
			ThreadProperty property;
		};

		std::shared_mutex infos_mutex;
		Status current_state = Status::Normal;
		std::size_t acceptable_mask = 0;
		std::pmr::vector<ThreadInfo> thread_infos;

		std::size_t const startup_acceptable_mask = 0;

	private:

		void ThreadExecute(ThreadProperty thread_property, std::stop_token& sk);
		//void Terminal(Status state, NodeSequencer& target_sequence, std::size_t group_id) noexcept;
		//bool ExecuteNodeSequencer(NodeSequencer& target_sequence, std::size_t group_id, TimeT::time_point now_time);
	};
}

namespace Potato::Task
{

	template<typename LanbdaT>
	struct LambdaNode : public Node, public Potato::IR::MemoryResourceRecordIntrusiveInterface
	{
		template<typename FunT>
		LambdaNode(Potato::IR::MemoryResourceRecord Record, FunT&& Fun)
			: MemoryResourceRecordIntrusiveInterface(Record), TaskInstance(std::forward<FunT>(Fun))
		{

		}

		virtual void TaskExecute(Context& context, Node& self, Node::Parameter& parameter) override
		{
			TaskInstance.operator()(context, self, parameter);
		}

	protected:

		virtual void AddTaskNodeRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubTaskNodeRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }

	private:

		LanbdaT TaskInstance;
	};

	template<AcceptableTaskNode FunT>
	Node::Ptr Context::CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource)
	{
		using Type = LambdaNode<std::remove_cvref_t<FunT>>;
		auto Record = Potato::IR::MemoryResourceRecord::Allocate<Type>(resource);
		if (Record)
		{
			return new (Record.Get()) Type{ Record, std::forward<FunT>(func) };
		}
		return {};
	}
}