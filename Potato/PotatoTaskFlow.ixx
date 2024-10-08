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

	struct TaskFlowNodeProperty
	{
		std::u8string_view display_name;
		TaskFilter filter;
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

		virtual bool Update() { return true; }

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) = 0;
		virtual void TaskFlowNodeTerminal(TaskProperty property) noexcept {}
		~TaskFlowNode() {assert(owner == 0 && index == std::numeric_limits<std::size_t>::max());}

	protected:

		TaskFlowNode() = default;

		std::atomic_size_t owner = 0;
		std::size_t index = std::numeric_limits<std::size_t>::max();


		virtual void AddTaskFlowNodeRef() const = 0;
		virtual void SubTaskFlowNodeRef() const = 0;

		friend struct TaskFlow;
	};

	export struct TaskFlowListener;

	export struct TaskFlow : public TaskFlowNode, protected Task
	{

		struct Wrapper
		{
			template<typename T>
			void AddRef(T* ptr) { ptr->AddTaskFlowRef(); }
			template<typename T>
			void SubRef(T* ptr) { ptr->SubTaskFlowRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow, Wrapper>;

		enum class EdgeType
		{
			Direct,
			//ReverseDirect,
			Mutex,
		};

		static TaskFlow::Ptr CreateTaskFlow(std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		template<typename Func>
		static TaskFlowNode::Ptr CreateLambdaTask(Func&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<Func, TaskFlowContext&>);

		bool AddNode(TaskFlowNode& node, TaskFlowNodeProperty property)
		{
			std::lock_guard lg(preprocess_mutex);
			return AddNode_AssumedLocked(node, property);
		}

		template<typename Func>
		TaskFlowNode::Ptr AddLambda(Func&& func, TaskFlowNodeProperty property, std::pmr::memory_resource *resource = std::pmr::get_default_resource()) requires(std::is_invocable_v<Func, TaskFlowContext&>)
		{
			auto ptr = CreateLambdaTask(std::forward<Func>(func), resource);
			if(ptr)
			{
				if(AddNode(*ptr, property))
				{
					return ptr;
				}
			}
			return {};
		}

		bool Remove(TaskFlowNode& node) { std::lock_guard lg(preprocess_mutex); return Remove_AssumedLocked(node); }
		bool AddDirectEdge(TaskFlowNode& from, TaskFlowNode& direct_to, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource()) { std::lock_guard lg(preprocess_mutex); return AddDirectEdge_AssumedLocked(from, direct_to , temp_resource); }
		bool AddMutexEdge(TaskFlowNode& from, TaskFlowNode& direct_to) { std::lock_guard lg(preprocess_mutex); return AddMutexEdge_AssumedLocked(from, direct_to); }
		bool RemoveDirectEdge(TaskFlowNode& from, TaskFlowNode& direct_to) { std::lock_guard lg(preprocess_mutex); return RemoveDirectEdge_AssumedLocked(from, direct_to); }

		virtual void TaskFlowExecuteBegin(TaskFlowContext& context) {}
		virtual void TaskFlowExecuteEnd(TaskFlowContext& context) {}

		bool Update() override
		{
			std::lock_guard lg(process_mutex);
			return Update_AssumedLocked();
		}
		virtual bool Commited(TaskContext& context, TaskFlowNodeProperty property)
		{
			std::lock_guard lg(process_mutex);
			return Commited_AssumedLocked(context, std::move(property));
		}
		virtual bool Commited(TaskContext& context, std::chrono::steady_clock::time_point time_point, TaskFlowNodeProperty property)
		{
			std::lock_guard lg(process_mutex);
			return Commited_AssumedLocked(context, std::move(property), time_point);
		}
		virtual bool MarkNodePause(std::size_t node_identity);
		virtual bool ContinuePauseNode(TaskContext& context, std::size_t node_identity);
		~TaskFlow();

		struct TemporaryNodeIndex
		{
			std::size_t current_index;
			std::size_t process_node_count;
		};

		template<typename Func>
		bool AddTemporaryNode(TaskFlowNode& node, TaskFlowNodeProperty property, Func&& func)
			requires(std::is_invocable_r_v<bool, Func, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex>)
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(node, property, std::forward<Func>(func));
		}

		bool AddTemporaryNode(TaskFlowNode& node, TaskFlowNodeProperty property)
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(node, property, nullptr, nullptr);
		}

	protected:
			
		TaskFlow(std::pmr::memory_resource* task_flow_resource = std::pmr::get_default_resource());

		bool AddNode_AssumedLocked(TaskFlowNode& node, TaskFlowNodeProperty property);

		virtual bool Update_AssumedLocked();
		bool Commited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property, std::chrono::steady_clock::time_point time_point);
		bool Commited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property);

		template<typename Func>
		bool AddTemporaryNode_AssumedLocked(TaskFlowNode& node, TaskFlowNodeProperty property, Func&& func)
			requires(std::is_invocable_r_v<bool, Func, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex>)
		{
			return this->AddTemporaryNode_AssumedLocked(node, property, [](void* append_data, TaskFlowNode const& node, TaskFlowNodeProperty pro, TemporaryNodeIndex index) -> bool
			{
				return (*static_cast<Func*>(append_data))(node, pro, index);
			}, &func);
		}

		bool AddTemporaryNode_AssumedLocked(TaskFlowNode& node, TaskFlowNodeProperty property, bool(*detect_func)(void* append_data, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex), void* append_data);
		bool SubTaskCommited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property);
		bool Remove_AssumedLocked(TaskFlowNode& node);
		bool AddDirectEdge_AssumedLocked(TaskFlowNode& from, TaskFlowNode& direct_to, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource());
		bool AddMutexEdge_AssumedLocked(TaskFlowNode& from, TaskFlowNode& direct_to);
		bool RemoveDirectEdge_AssumedLocked(TaskFlowNode& from, TaskFlowNode& direct_to);

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) override;
		virtual void TaskExecute(TaskContextWrapper& status) override;

		virtual void AddTaskFlowRef() const = 0;
		virtual void SubTaskFlowRef() const = 0;
		virtual void AddTaskRef() const override { AddTaskFlowRef(); }
		virtual void SubTaskRef() const override { SubTaskFlowRef(); }
		void AddTaskFlowNodeRef() const override { AddTaskFlowRef(); }
		void SubTaskFlowNodeRef() const override { SubTaskFlowRef(); }

		struct PreprocessMutexEdge
		{
			std::size_t node1;
			std::size_t node2;
		};

		struct PreprocessDirectEdge
		{
			std::size_t from;
			std::size_t to;
		};

		struct PreprocessNode
		{
			TaskFlowNode::Ptr node;
			std::size_t in_degree = 0;
			std::size_t topology_degree = 0;
			TaskFlowNodeProperty property;
		};

		std::mutex preprocess_mutex;
		std::pmr::vector<PreprocessNode> preprocess_nodes;
		std::pmr::vector<PreprocessMutexEdge> preprocess_mutex_edges;
		std::pmr::vector<PreprocessDirectEdge> preprocess_direct_edges;
		bool need_update = false;

		enum class Status
		{
			READY,
			RUNNING,
			RUNNING_NEED_PAUSE,
			PAUSE,
			DONE
		};

		struct ProcessNode
		{
			Status status = Status::READY;
			std::size_t in_degree = 0;
			std::size_t mutex_degree = 0;
			Misc::IndexSpan<> direct_edges;
			Misc::IndexSpan<> mutex_edges;
			std::size_t init_in_degree = 0;
			TaskFlowNode::Ptr node;
			TaskFlowNodeProperty property;
			std::size_t pause_count = 0;
			bool need_append_mutex = false;
		};

		bool TryStartupNode_AssumedLock(TaskContext& context, ProcessNode& node, std::size_t index);
		bool FinishNode_AssumedLock(TaskContext& context, ProcessNode& node, std::size_t index);

		std::mutex process_mutex;
		Status current_status = Status::READY;
		std::pmr::vector<ProcessNode> process_nodes;
		std::pmr::vector<std::size_t> process_edges;

		struct AppendEdge
		{
			std::size_t from;
			std::size_t to;
		};

		std::pmr::vector<AppendEdge> append_direct_edge;

		std::size_t finished_task = 0;
		std::size_t temporary_node_offset = 0;
		std::size_t temporary_edge_offset = 0;
		TaskFlowNodeProperty running_property;

		std::mutex parent_mutex;
		TaskFlow::Ptr parent_node;
		std::size_t current_identity = 0;

		friend struct Task::Wrapper;
	};

	struct TaskFlowContext
	{
		TaskContext& context;
		Status status = Status::Normal;
		TaskFlowNodeProperty node_property;
		ThreadProperty thread_property;
		TaskFlowNode::Ptr current_node;
		TaskFlow::Ptr flow;
		std::size_t node_identity = 0;
	};

	

	
}

namespace
{
	struct BuildInTaskFlow : public Potato::Task::TaskFlow, public Potato::IR::MemoryResourceRecordIntrusiveInterface
	{
		BuildInTaskFlow(Potato::IR::MemoryResourceRecord record) : MemoryResourceRecordIntrusiveInterface(record) {}
		void SubTaskFlowRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }
		void AddTaskFlowRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
	};

	template<typename Func>
	struct LambdaTaskFlowNode : public Potato::Task::TaskFlowNode, protected Potato::IR::MemoryResourceRecordIntrusiveInterface
	{
		Func func;

		void AddTaskFlowNodeRef() const override{ MemoryResourceRecordIntrusiveInterface::AddRef(); }
		void SubTaskFlowNodeRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }

		void TaskFlowNodeExecute(Potato::Task::TaskFlowContext& status) override { func(status); }

		LambdaTaskFlowNode(Potato::IR::MemoryResourceRecord record, Func func)
			: MemoryResourceRecordIntrusiveInterface(record), func(std::move(func)) {}
	};
}

namespace Potato::Task
{
	template<typename Fun>
	auto TaskFlow::CreateLambdaTask(Fun&& func, std::pmr::memory_resource* resource)
		->TaskFlowNode::Ptr
		requires(std::is_invocable_v<Fun, TaskFlowContext&>)
	{
		using Type = LambdaTaskFlowNode<std::remove_cvref_t<Fun>>;
		return IR::MemoryResourceRecord::AllocateAndConstruct<Type>(resource, std::forward<Fun>(func));
	}

	inline TaskFlow::Ptr TaskFlow::CreateTaskFlow(std::pmr::memory_resource* resource)
	{
		return IR::MemoryResourceRecord::AllocateAndConstruct<BuildInTaskFlow>(resource);
	}
}