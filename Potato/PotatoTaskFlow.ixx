module;

#include <cassert>

export module PotatoTaskFlow;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;
import PotatoTaskSystem;


export namespace Potato::Task
{
	struct TaskFlowExecute;

	export struct TaskFlow;

	struct NodeProperty
	{
		std::u8string_view display_name;
		std::optional<TaskFilter> filter;
	};

	export struct TaskFlowContext;

	struct TaskFlowNode
	{
		struct Wrapper
		{
			void AddRef(TaskFlowNode const* ptr) { ptr->AddTaskFlowNodeRef(); }
			void SubRef(TaskFlowNode const* ptr) { ptr->SubTaskFlowNodeRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlowNode, TaskFlowNode::Wrapper>;

		template<typename Fun>
		static auto CreateLambda(Fun&& func, std::pmr::memory_resource* resouce = std::pmr::get_default_resource())
			-> Ptr requires(std::is_invocable_v<Fun, TaskFlowContext&>);

		virtual void Update() {}

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) = 0;
		virtual void TaskFlowNodeTerminal(TaskProperty property) noexcept {}

	protected:

		virtual void AddTaskFlowNodeRef() const = 0;
		virtual void SubTaskFlowNodeRef() const = 0;
	};

	struct TaskFlowNodeProcessor
	{
		struct Wrapper
		{
			void AddRef(TaskFlowNodeProcessor const* p) const { p->AddTaskFlowNodeProcessorRef(); }
			void SubRef(TaskFlowNodeProcessor const* p) const { p->SubTaskFlowNodeProcessorRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlowExecute, Wrapper>;

		virtual bool Commit(TaskContext& context, std::optional<std::chrono::steady_clock::time_point> delay_point = std::nullopt) { return false; }

	protected:

		virtual void AddTaskFlowNodeProcessorRef() const = 0;
		virtual void SubTaskFlowNodeProcessorRef() const = 0;
	};

	export struct TaskFlow : public TaskFlowNode
	{

		struct Wrapper
		{
			void AddRef(TaskFlow const* ptr) { ptr->AddTaskFlowRef(); }
			void SubRef(TaskFlow const* ptr) { ptr->SubTaskFlowRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow, Wrapper>;

		struct Node : protected Pointer::DefaultIntrusiveInterface
		{
			using Ptr = Pointer::IntrusivePtr<Node>;

			static Ptr Create(
				TaskFlow* owner,
				TaskFlowNode::Ptr reference_node,
				NodeProperty property,
				std::size_t index, 
				std::pmr::memory_resource* resource);

		protected:

			Node(IR::MemoryResourceRecord record, Pointer::ObserverPtr<TaskFlow> owner, TaskFlowNode::Ptr reference_node, NodeProperty property, std::size_t index);

			void Release() override;

			IR::MemoryResourceRecord record;
			Pointer::ObserverPtr<TaskFlow> owner;
			TaskFlowNode::Ptr reference_node;
			NodeProperty property;

			std::mutex mutex;
			std::size_t reference_id;

			friend struct Pointer::DefaultIntrusiveWrapper;
			friend struct TaskFlow;
		};

		TaskFlowNodeProcessor::Ptr CreateProcessor();

		Node::Ptr AddNode(TaskFlowNode::Ptr node, NodeProperty property = {});
		

		template<typename Fun>
		TaskFlowNode::Ptr AddLambda(Fun&& func, std::u8string_view display_name = {}, std::optional<TaskProperty> property = std::nullopt, std::pmr::memory_resource* resouce = std::pmr::get_default_resource())
			requires(std::is_invocable_v<Fun, TaskFlowContext&>)
		{
			auto ptr = TaskFlowNode::CreateLambda(std::forward<Fun>(func), resouce);
			if (AddNode(ptr, property))
			{
				return ptr;
			}
			return {};
		}

		bool Remove(Node& node);
		bool AddDirectEdge(Node& form, Node& direct_to, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource());
		bool AddMutexEdge(Node& form, Node& direct_to);
		bool RemoveDirectEdge(Node& form, Node& direct_to);

		//void Update() override;

		//bool TryUpdate(std::pmr::vector<ErrorNode>* error_output = nullptr, std::pmr::memory_resource* temp = std::pmr::get_default_resource());
		//TaskFlowExecute::Ptr Commit(TaskContext& context, TaskProperty property = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		//Ptr CommitDelay(TaskContext& context, std::u8string_view display_name = {}, std::chrono::steady_clock::time_point time_point, TaskProperty property = {});
		bool Remove(TaskFlowNode::Ptr node);

		struct MemorySetting
		{
			std::pmr::memory_resource* processor_resource = std::pmr::get_default_resource();
			std::pmr::memory_resource* node_resource = std::pmr::get_default_resource();
		};

		TaskFlow(std::pmr::memory_resource* task_flow_resource = std::pmr::get_default_resource(), MemorySetting memory_setting = {});

		virtual void TaskFlowExecuteBegin(ExecuteStatus& status, TaskFlowExecute& execute) {}
		virtual void TaskFlowExecuteEnd(ExecuteStatus& status, TaskFlowExecute& execute) {}

	protected:

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) override;

		virtual void AddTaskFlowRef() const = 0;
		virtual void SubTaskFlowRef() const = 0;
		void AddTaskFlowNodeRef() const override { AddTaskFlowRef(); }
		void SubTaskFlowNodeRef() const override { SubTaskFlowNodeRef(); }

		enum class EdgeType
		{
			Direct,
			//ReverseDirect,
			Mutex,
		};

		struct RawEdge
		{
			EdgeType type = EdgeType::Direct;
			std::size_t from;
			std::size_t to;
		};

		struct RawNode
		{
			Node::Ptr reference_node;
			std::size_t in_degree = 0;
		};

		MemorySetting resources;

		std::mutex raw_mutex;
		std::pmr::vector<RawNode> raw_nodes;
		std::pmr::vector<RawEdge> raw_edges;
		bool need_update = false;

		struct ProcessNode
		{
			RawNode pre_node;
			std::size_t in_degree = 0;
			Misc::IndexSpan<> direct_edges;
			Misc::IndexSpan<> mutex_edges;
		};

		std::mutex process_mutex;
		std::size_t reference_processor = 0;
		std::pmr::vector<ProcessNode> process_nodes;
		std::pmr::vector<std::size_t> process_edges;

		friend struct Task::Wrapper;
	};

	struct TaskFlowProcessor
	{
		
	};

	struct TaskFlowContext
	{
		
	};

	/*
	struct TaskNodeExecuteNodes
	{
		enum class RunningState
		{
			Idle,
			Running,
			Done,
			RequireStop,
		};

		struct ExecuteNode
		{
			TaskFlowNode::Ptr node;
			TaskProperty property;
			std::size_t require_in_degree = 0;
			std::size_t current_in_degree = 0;
			std::size_t mutex_count = 0;
			Misc::IndexSpan<> mutex_span;
			Misc::IndexSpan<> directed_span;
			RunningState status = RunningState::Idle;
		};

		std::pmr::vector<ExecuteNode> nodes;
		std::pmr::vector<std::size_t> edges;

		TaskNodeExecuteNodes(std::pmr::memory_resource* resource)
			: nodes(resource), edges(resource)
		{
			
		}

	};
	*/


	

	/*
	struct IndependenceTaskFlowExecute : public TaskFlowExecute, public Task, public Potato::Pointer::DefaultIntrusiveInterface
	{
		virtual bool Commit(TaskContext& context, std::optional<std::chrono::steady_clock::time_point> delay_point) override;
		virtual bool Reset() override;
		virtual bool ReCloneNode() override;
	protected:

		virtual void AddTaskFlowExecuteRef() const override { DefaultIntrusiveInterface::AddRef(); }
		virtual void SubTaskFlowExecuteRef() const override { DefaultIntrusiveInterface::SubRef(); }
		virtual void AddTaskRef() const override { DefaultIntrusiveInterface::AddRef(); }
		virtual void SubTaskRef() const override { DefaultIntrusiveInterface::SubRef(); }
		virtual void Release() override;
		std::size_t StartupTaskFlow(TaskContext& context);

		IndependenceTaskFlowExecute(TaskFlow::Ptr owner, Potato::IR::MemoryResourceRecord record, TaskProperty property);
		
		void TaskExecute(ExecuteStatus& status) override;

		using RunningState = TaskNodeExecuteNodes::RunningState;

		TaskFlow::Ptr owner;
		Potato::IR::MemoryResourceRecord record;
		TaskProperty property;

		std::mutex mutex;
		TaskNodeExecuteNodes nodes;
		std::size_t run_task = 0;
		RunningState state = RunningState::Idle;

		bool TryStartSingleTaskFlowImp(TaskContext& context, std::size_t fast_index);
		std::size_t FinishTaskFlowNode(TaskContext& context, std::size_t fast_index);
		friend struct TaskFlow;
	};
	*/


	template<typename Func>
	struct LambdaTaskFlowNode : public TaskFlowNode, protected Pointer::DefaultIntrusiveInterface
	{
		IR::MemoryResourceRecord record;
		Func func;

		void AddTaskFlowNodeRef() const override{ DefaultIntrusiveInterface::AddRef(); }
		void SubTaskFlowNodeRef() const override { DefaultIntrusiveInterface::SubRef(); }

		void Release() override
		{
			auto re = record;
			this->~LambdaTaskFlowNode();
			re.Deallocate();
		}

		void TaskFlowNodeExecute(TaskFlowContext& status) override { func(status); }

		LambdaTaskFlowNode(IR::MemoryResourceRecord record, Func func)
			: record(record), func(std::move(func)) {}
	};

	template<typename Fun>
	auto TaskFlowNode::CreateLambda(Fun&& func, std::pmr::memory_resource* resouce) ->TaskFlowNode::Ptr requires(std::is_invocable_v<Fun, TaskFlowContext&>)
	{
		using Type = LambdaTaskFlowNode<std::remove_cvref_t<Fun>>;
		assert(resouce != nullptr);
		auto record = Potato::IR::MemoryResourceRecord::Allocate<Type>(resouce);
		if (record)
		{
			return new (record.Get()) Type{ record, std::forward<Fun>(func) };
		}
		return {};
	}
}