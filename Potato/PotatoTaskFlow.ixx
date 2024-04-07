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

	export struct TaskFlow;

	/*
	struct TaskFlowPause
	{
		TaskFlow& owner;
		std::size_t self_index;
		TaskProperty property;

		void Continue(TaskContext& context);
	};
	*/

	struct TaskFlowStatus
	{
		TaskContext& context;
		TaskProperty task_property;
		ThreadProperty thread_property;
		TaskFlow& owner;
		std::size_t self_index = 0;
		std::u8string_view display_name;
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

		bool InsideTaskFlow() const { std::lock_guard lg(node_mutex); return owner != nullptr; }

	protected:

		virtual void AddTaskFlowNodeRef() const = 0;
		virtual void SubTaskFlowNodeRef() const = 0;

		virtual void TaskFlowNodeExecute(TaskFlowStatus& status) = 0;
		virtual void TaskFlowNodeTerminal(TaskProperty property) noexcept {}

		std::size_t GetFastIndex() const { std::lock_guard lg(node_mutex); return fast_index; }

		friend struct TaskFlow;

	private:

		mutable std::mutex node_mutex;
		std::size_t fast_index = std::numeric_limits<std::size_t>::max();
		TaskFlow* owner = nullptr;
	};

	export struct TaskFlow : protected Task
	{

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow, Task::Wrapper>;

		static Ptr CreateDefaultTaskFlow(std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		template<typename Fun>
		TaskFlowNode::Ptr AddLambda(Fun&& func, TaskProperty property = {}, std::u8string_view display_name = {}, std::pmr::memory_resource* resouce = std::pmr::get_default_resource()) requires(std::is_invocable_v<Fun, TaskFlowStatus&>)
		{
			auto ptr = TaskFlowNode::CreateLambda(std::forward<Fun>(func), resouce);
			if(AddNode(ptr, property, display_name))
			{
				return ptr;
			}
			return {};
		}

		bool AddNode(TaskFlowNode::Ptr ptr, TaskProperty property = {}, std::u8string_view display_name = {});

		bool AddDirectEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to);
		bool AddMutexEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to);

		virtual ~TaskFlow();
		bool ResetState();

		struct PausePoint
		{
			void Continue(TaskContext& context);
			TaskFlow::Ptr ptr;
			std::size_t ref_index;
			TaskProperty property;
		};

		std::optional<PausePoint> CreatePause(TaskFlowStatus const& status);
		bool Update(bool reset_state = false, std::pmr::vector<TaskFlowNode::Ptr>* error_output = nullptr, std::pmr::memory_resource* temp = std::pmr::get_default_resource());
		bool Commit(TaskContext& context, TaskProperty property = {}, std::u8string_view display_name = {});
		bool CommitDelay(TaskContext& context, std::chrono::steady_clock::time_point time_point, TaskProperty property = {}, std::u8string_view display_name = {});
		bool Remove(TaskFlowNode::Ptr form);

		TaskFlow(std::pmr::memory_resource* resource = std::pmr::get_default_resource());

	protected:

		virtual void OnPostCommit() {}
		virtual void OnBeginTaskFlow(ExecuteStatus& status) {}
		virtual void OnFinishTaskFlow(ExecuteStatus& status) {}

		std::size_t FinishTaskFlowNode(TaskContext& context, std::size_t fast_index);
		bool TryStartSingleTaskFlowImp(TaskContext& context, std::size_t fast_index);
		TaskProperty RemoveNodeImp(TaskFlowNode& node);
		std::size_t StartupTaskFlow(TaskContext& context);

		virtual void TaskExecute(ExecuteStatus& status) override;
		virtual void TaskTerminal(TaskProperty property, AppendData data) noexcept override;

		enum class EdgeType
		{
			Direct,
			ReverseDirect,
			Mutex,
		};

		struct PreCompiledEdge
		{
			EdgeType type = EdgeType::Direct;
			Potato::Pointer::ObserverPtr<TaskFlowNode> node;
		};

		struct PreCompiledNode
		{
			TaskFlowNode::Ptr node;
			TaskProperty property;
			std::pmr::vector<PreCompiledEdge> edges;
			std::size_t in_degree = 0;
			std::size_t out_degree = 0;
			std::size_t edge_count = 0;
			bool updated = false;
			bool require_remove = false;
			std::u8string_view display_name;
		};

		enum class RunningState
		{
			Idle,
			Running,
			Done,
			RequireStop,
		};

		struct CompiledNode
		{
			TaskFlowNode::Ptr ptr;
			RunningState status = RunningState::Idle;
			std::size_t require_in_degree = 0;
			std::size_t current_in_degree = 0;
			std::size_t mutex_count = 0;
			Misc::IndexSpan<> mutex_span;
			Misc::IndexSpan<> directed_span;
			TaskProperty property;
			std::size_t require_stop_record;
			std::u8string_view display_name;
		};

		std::mutex pre_compiled_mutex;
		std::pmr::monotonic_buffer_resource temp_resource;
		std::pmr::vector<PreCompiledNode> pre_compiled_nodes;
		std::size_t require_remove_count = 0;
		bool need_update = false;

		std::mutex compiled_mutex;
		std::pmr::vector<CompiledNode> compiled_nodes;
		std::pmr::vector<std::size_t> compiled_edges;
		std::size_t exist_task = 0;
		std::size_t run_task = 0;
		RunningState running_state = RunningState::Idle;


		friend struct Task::Wrapper;
	};

	


	/*
	struct TaskFlowGraphic
	{
		
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




	/*
	export struct TaskFlow : public TaskFlowNode, public Task
	{
		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow>;

		TaskFlow(std::pmr::memory_resource* mr);

		bool Commit(TaskContext& context, TaskProperty property);

		bool Pause(TaskFlowNode* target);
		bool Continue(TaskContext& context, TaskProperty property, TaskFlowNode* target);

	protected:

		virtual void TaskExecute(ExecuteStatus& status) override;
		virtual void TaskFlowNodeExecute(TaskFlowStatus& status) override;

		virtual void OnStartTaskFlow(ExecuteStatus& context) {};
		virtual void OnEndTaskFlow(ExecuteStatus& context) {};

		void StartTaskFlow(TaskContext& context, TaskProperty property);
		void FinishTask(TaskContext& context, std::size_t index, TaskProperty property);

		enum class Status
		{
			Idle,
			Waiting,
			Baked,
			Running
		};

		struct Node
		{
			TaskFlowNode::Ptr node;
			std::size_t in_degree;
			std::size_t mutex_degree;
			Misc::IndexSpan<> edge_index;
		};

		struct Edge
		{
			bool is_mutex;
			std::size_t from;
			std::size_t to;
		};

		mutable std::mutex flow_mutex;
		Status status = Status::Idle;
		std::pmr::vector<Node> static_nodes;
		std::pmr::vector<Edge> static_edges;
		TaskFlow::Ptr owner;
		TaskProperty owner_property;
		//std::pmr::vector<TaskFlowPause> pause;

		friend struct Potato::Pointer::DefaultIntrusiveWrapper;
		friend struct Builder;
	};

	struct DefaultTaskFlow : public TaskFlow
	{
		
	};
	*/






}