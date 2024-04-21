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

	struct TaskFlowStatus
	{
		TaskContext& context;
		TaskProperty task_property;
		ThreadProperty thread_property;
		TaskFlowExecute& executor;
		TaskFlow& owner;
		std::size_t self_index = 0;
	};

	struct TaskFlowNode
	{
		struct Wrapper
		{
			template<typename T>
			void AddRef(T* ptr) { ptr->AddTaskFlowNodeRef(); }
			template<typename T>
			void SubRef(T* ptr) { ptr->SubTaskFlowNodeRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlowNode, TaskFlowNode::Wrapper>;

		template<typename Fun>
		static auto CreateLambda(Fun&& func, std::pmr::memory_resource* resouce = std::pmr::get_default_resource())
			-> Ptr requires(std::is_invocable_v<Fun, TaskFlowStatus&>);

		TaskFlowNode::Ptr Clone() { return this; }

		virtual void TaskFlowNodeExecute(TaskFlowStatus& status) = 0;
		virtual void TaskFlowNodeTerminal(TaskProperty property) noexcept {}

	protected:

		virtual void AddTaskFlowNodeRef() const = 0;
		virtual void SubTaskFlowNodeRef() const = 0;
	};

	struct TaskFlowExecute
	{
		struct Wrapper
		{
			template<typename T> void AddRef(T* p) const { p->AddTaskFlowExecuteRef(); }
			template<typename T> void SubRef(T* p) const { p->SubTaskFlowExecuteRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlowExecute, Wrapper>;

		virtual bool Commit(TaskContext& context, std::optional<std::chrono::steady_clock::time_point> delay_point = std::nullopt) { return false; }
		virtual bool Reset() { return false; };
		virtual bool ReCloneNode() { return false; }

	protected:

		virtual void AddTaskFlowExecuteRef() const = 0;
		virtual void SubTaskFlowExecuteRef() const = 0;
	};

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


	export struct TaskFlow
	{

		struct Wrapper
		{
			template<typename T> void AddRef(T* ptr) { ptr->AddTaskFlowRef(); }
			template<typename T> void SubRef(T* ptr) { ptr->SubTaskFlowRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow, Wrapper>;

		template<typename Fun>
		TaskFlowNode::Ptr AddLambda(Fun&& func, std::u8string_view display_name = {}, std::optional<TaskProperty> property = std::nullopt, std::pmr::memory_resource* resouce = std::pmr::get_default_resource()) requires(std::is_invocable_v<Fun, TaskFlowStatus&>)
		{
			auto ptr = TaskFlowNode::CreateLambda(std::forward<Fun>(func), resouce);
			if(AddNode(ptr, display_name, property))
			{
				return ptr;
			}
			return {};
		}

		bool AddNode(TaskFlowNode::Ptr node, NodeProperty property = {});

		bool AddDirectEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to);
		bool AddMutexEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to);

		struct ErrorNode
		{
			TaskFlowNode::Ptr node;
			std::u8string_view display_name;
		};

		bool TryUpdate(std::pmr::vector<ErrorNode>* error_output = nullptr, std::pmr::memory_resource* temp = std::pmr::get_default_resource());
		TaskFlowExecute::Ptr Commit(TaskContext& context, TaskProperty property = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		//Ptr CommitDelay(TaskContext& context, std::u8string_view display_name = {}, std::chrono::steady_clock::time_point time_point, TaskProperty property = {});
		bool Remove(TaskFlowNode::Ptr node);

		TaskFlow(std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		bool CloneCompliedNodes(TaskNodeExecuteNodes& nodes, TaskFilter ref_filter = {});
		virtual void TaskFlowExecuteBegin(ExecuteStatus& status, TaskFlowExecute& execute) {}
		virtual void TaskFlowExecuteEnd(ExecuteStatus& status, TaskFlowExecute& execute) {}

	protected:

		virtual void AddTaskFlowRef() const {}
		virtual void SubTaskFlowRef() const {}

		std::optional<std::size_t> LocatePreCompliedNode(TaskFlowNode& node, std::lock_guard<std::mutex> const& lg) const;

		enum class EdgeType
		{
			Direct,
			//ReverseDirect,
			Mutex,
		};

		struct PreCompiledEdge
		{
			EdgeType type = EdgeType::Direct;
			std::size_t from;
			std::size_t to;
		};

		struct PreCompiledNode
		{
			TaskFlowNode::Ptr node;
			NodeProperty property;
		};

		std::mutex pre_compiled_mutex;
		std::pmr::vector<PreCompiledNode> pre_complied_nodes;
		std::pmr::vector<PreCompiledEdge> pre_complied_edges;
		bool need_update = false;

		struct CompiledNode
		{
			PreCompiledNode pre_node;
			std::size_t in_degree = 0;
			Misc::IndexSpan<> direct_edges;
			Misc::IndexSpan<> mutex_edges;
		};

		std::mutex compiled_mutex;
		std::pmr::vector<CompiledNode> complied_nodes;
		std::pmr::vector<std::size_t> complied_edges;

		std::atomic_size_t run_task = 0;

		friend struct Task::Wrapper;
	};

	
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

		void TaskFlowNodeExecute(TaskFlowStatus& status) override { func(status); }

		LambdaTaskFlowNode(IR::MemoryResourceRecord record, Func func)
			: record(record), func(std::move(func)) {}
	};

	template<typename Fun>
	auto TaskFlowNode::CreateLambda(Fun&& func, std::pmr::memory_resource* resouce) ->TaskFlowNode::Ptr requires(std::is_invocable_v<Fun, TaskFlowStatus&>)
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