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

	struct NodeProperty
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

		template<typename Fun>
		static auto CreateLambda(Fun&& func, std::pmr::memory_resource* resouce = std::pmr::get_default_resource())
			-> Ptr requires(std::is_invocable_v<Fun, TaskFlowContext&>);

		virtual bool Update() { return true; }

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) = 0;
		virtual void TaskFlowNodeTerminal(TaskProperty property) noexcept {}

	protected:

		virtual void AddTaskFlowNodeRef() const = 0;
		virtual void SubTaskFlowNodeRef() const = 0;
	};

	/*
	struct TaskFlowNodeProcessor : public Task
	{
		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlowNodeProcessor, Task::Wrapper>;
	protected:
	};
	*/

	export struct TaskFlowListener;

	export struct TaskFlow : public TaskFlowNode, protected Task
	{

		struct Wrapper
		{
			void AddRef(TaskFlow const* ptr) { ptr->AddTaskFlowRef(); }
			void SubRef(TaskFlow const* ptr) { ptr->SubTaskFlowRef(); }
		};

		struct UserData
		{
			struct Wrapper
			{
				void AddRef(UserData const* p) const { p->AddUserDataRef(); }
				void SubRef(UserData const* p) const { p->SubUserDataRef(); }
			};

			using Ptr = Pointer::IntrusivePtr<UserData, Wrapper>;

		protected:

			virtual void AddUserDataRef() const = 0;
			virtual void SubUserDataRef() const = 0;
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow, Wrapper>;

		struct Node : protected Pointer::DefaultIntrusiveInterface
		{
			using Ptr = Pointer::IntrusivePtr<Node>;

			static Ptr Create(
				TaskFlow* owner, 
				TaskFlowNode::Ptr reference_node,
				UserData::Ptr user_data,
				NodeProperty property, 
				std::size_t index, 
				std::pmr::memory_resource* resource
			);

		protected:

			Node(
				IR::MemoryResourceRecord record, 
				Pointer::ObserverPtr<TaskFlow> owner, 
				TaskFlowNode::Ptr reference_node,
				UserData::Ptr user_data,
				TaskFilter filter, 
				std::u8string_view display_name, 
				std::size_t index
			);

			void Release() override;

			IR::MemoryResourceRecord record;
			Pointer::ObserverPtr<TaskFlow> owner;
			TaskFlowNode::Ptr reference_node;
			TaskFilter filter;
			std::u8string_view display_name;
			UserData::Ptr user_data;

			std::mutex mutex;
			std::size_t reference_id;

			friend struct Pointer::DefaultIntrusiveWrapper;
			friend struct TaskFlow;
		};

		Node::Ptr AddNode(TaskFlowNode::Ptr node, NodeProperty property = {}, UserData::Ptr user_data = {})
		{
			std::lock_guard lg(raw_mutex);
			return AddNode_AssumedLock(std::move(node), std::move(property), std::move(user_data));
		}
		

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

		bool Remove(Node& node) { std::lock_guard lg(raw_mutex); return Remove(node); }
		bool AddDirectEdge(Node& from, Node& direct_to, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource()) { std::lock_guard lg(raw_mutex); return AddDirectEdge_AssumedLock(from, direct_to , temp_resource); }
		bool AddMutexEdge(Node& from, Node& direct_to) { std::lock_guard lg(raw_mutex); return AddMutexEdge_AssumedLock(from, direct_to); }
		bool RemoveDirectEdge(Node& from, Node& direct_to) { std::lock_guard lg(raw_mutex); return RemoveDirectEdge_AssumedLock(from, direct_to); }

		struct MemorySetting
		{
			std::pmr::memory_resource* processor_resource = std::pmr::get_default_resource();
			std::pmr::memory_resource* node_resource = std::pmr::get_default_resource();
		};

		TaskFlow(std::chrono::steady_clock::duration listen_duration = std::chrono::microseconds{100}, std::pmr::memory_resource* task_flow_resource = std::pmr::get_default_resource(), MemorySetting memory_setting = {});

		virtual void TaskFlowExecuteBegin(TaskFlowContext& context) {}
		virtual void TaskFlowExecuteEnd(TaskFlowContext& context) {}

		bool Update() override
		{
			std::lock_guard lg(process_mutex);
			return Update_AssumedLock();
		}
		virtual bool Commited(TaskContext& context, NodeProperty property)
		{
			std::lock_guard lg(process_mutex);
			return Commited_AssumedLock(context, std::move(property));
		}
		virtual bool Commited(TaskContext& context, std::chrono::steady_clock::time_point time_point, NodeProperty property)
		{
			std::lock_guard lg(process_mutex);
			return Commited_AssumedLock(context, std::move(property), time_point);
		}
		virtual bool MarkNodePause(std::size_t node_identity);
		virtual bool ContinuePauseNode(TaskContext& context, std::size_t node_identity);

	protected:

		bool Update_AssumedLock();
		bool Commited_AssumedLock(TaskContext& context, NodeProperty property, std::chrono::steady_clock::time_point time_point);
		bool Commited_AssumedLock(TaskContext& context, NodeProperty property);
		Node::Ptr AddNode_AssumedLock(TaskFlowNode::Ptr node, NodeProperty property = {}, UserData::Ptr user_data = {});
		bool Remove_AssumedLock(Node& node);
		bool AddDirectEdge_AssumedLock(Node& from, Node& direct_to, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource());
		bool AddMutexEdge_AssumedLock(Node& from, Node& direct_to);
		bool RemoveDirectEdge_AssumedLock(Node& from, Node& direct_to);

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) override;
		virtual void TaskExecute(ExecuteStatus& status) override;

		virtual void AddTaskFlowRef() const = 0;
		virtual void SubTaskFlowRef() const = 0;
		virtual void AddTaskRef() const override { AddTaskFlowRef(); }
		virtual void SubTaskRef() const override { SubTaskFlowRef(); }
		void AddTaskFlowNodeRef() const override { AddTaskFlowRef(); }
		void SubTaskFlowNodeRef() const override { SubTaskFlowRef(); }
		

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

		enum class Status
		{
			READY,
			RUNNING,
			PAUSE,
			PAUSE_RUNNING,
			DONE
		};

		//virtual Status ExecuteTaskFlowNode(ExecuteStatus& status, TaskFlowNodeProcessor& processor, NodeProperty property);

		struct ProcessNode
		{
			Status status = Status::READY;
			std::size_t in_degree = 0;
			std::size_t mutex_degree = 0;
			Misc::IndexSpan<> direct_edges;
			Misc::IndexSpan<> mutex_edges;
			std::size_t init_in_degree = 0;
			Node::Ptr reference_node;
		};

		bool TryStartupNode(TaskContext& context, ProcessNode& node, std::size_t index);
		bool FinishNode_AssumedLock(TaskContext& context, ProcessNode& node);

		std::mutex process_mutex;
		Status current_status = Status::READY;
		std::pmr::vector<ProcessNode> process_nodes;
		std::pmr::vector<std::size_t> process_edges;
		std::size_t finished_task = 0;
		NodeProperty running_property;
		std::chrono::steady_clock::duration listen_duration;

		std::mutex parent_mutex;
		TaskFlow::Ptr parent_node;
		std::size_t current_identity = 0;

		friend struct Task::Wrapper;
	};

	struct TaskFlowContext
	{
		TaskContext& context;
		Status status = Status::Normal;
		NodeProperty node_property;
		ThreadProperty thread_property;
		TaskFlowNode::Ptr current_node;
		TaskFlow::Ptr flow;
		std::size_t node_identity = 0;
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