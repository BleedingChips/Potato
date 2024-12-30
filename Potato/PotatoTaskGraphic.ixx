module;

#include <cassert>

export module PotatoTaskGraphic;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;
import PotatoTask;
import PotatoGraph;


export namespace Potato::TaskGraphic
{

	using Graph::GraphNode;
	using Graph::CheckOptimize;
	using Graph::EdgeOptimize;
	using Graph::GraphEdge;

	export struct Flow;

	struct FlowNodeDetectionIndex
	{
		std::size_t current_index;
		std::size_t process_node_count;
	};

	template<typename Type>
	concept AcceptableTemporaryDetectFunction = std::is_invocable_r_v<bool, Type, Task::Node const&, Task::Property const&, FlowNodeDetectionIndex const&>;

	struct FlowProcessContext : public Task::NodeData
	{
		struct Wrapper
		{
			void AddRef(FlowProcessContext const* ptr) { ptr->AddTaskGraphicFlowProcessContextRef(); }
			void SubRef(FlowProcessContext const* ptr) { ptr->SubTaskGraphicFlowProcessContextRef(); }
		};
		using Ptr = Pointer::IntrusivePtr<FlowProcessContext, Wrapper>;

		template<AcceptableTemporaryDetectFunction Func>
		bool AddTemporaryNode(Task::Node::Ptr node, Func&& func, Task::Property property = {})
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(std::move(node), std::forward<Func>(func), std::move(property));
		}

		bool AddTemporaryNode(Task::Node::Ptr node, Task::Property property = {})
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(std::move(node), std::move(property), nullptr, nullptr);
		}

		struct Config
		{
			std::pmr::memory_resource* self_resource = std::pmr::get_default_resource();
			std::pmr::memory_resource* temporary_update_resource = std::pmr::get_default_resource();
		};

		~FlowProcessContext();

	protected:

		FlowProcessContext(Config config);
		bool UpdateFromFlow_AssumedLocked(Flow& flow);
		template<AcceptableTemporaryDetectFunction Func>
		bool AddTemporaryNode_AssumedLocked(Task::Node::Ptr node, Func&& func, Task::Property property = {})
		{
			return this->AddTemporaryNode_AssumedLocked(node, std::move(property), [](void* append_data, Task::Node const& node, Task::Property const& property, FlowNodeDetectionIndex const& index) -> bool
				{
					return (*static_cast<Func*>(append_data))(node, property, index);
				}, &func);
		}

		bool Start(Flow& flow, Task::ContextWrapper& wrapper);
		void OnTaskFlowTerminal(Flow& flow, Task::ContextWrapper& wrapper) noexcept {}
		bool OnTaskFlowFlowTriggerExecute(Flow& flow, Task::ContextWrapper& wrapper);
		bool OnTaskFlowFlowTriggerTerminal(Flow& flow, Task::ContextWrapper& wrapper) noexcept { return true; }
		bool PauseAndLaunch(Task::ContextWrapper& wrapper, Flow& flow, std::size_t index, Task::Node& ptr, Task::Property property, std::optional<std::chrono::steady_clock::time_point> delay_time);
		bool AddTemporaryNode_AssumedLocked(Task::Node::Ptr node, Task::Property property, bool(*detect_func)(void* append_data, Task::Node const&, Task::Property const&, FlowNodeDetectionIndex const& index), void* append_data);
		//bool SubTaskCommited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property);

		enum class State
		{
			READY,
			RUNNING,
			RUNNING_NEED_PAUSE,
			PAUSE,
			DONE
		};

		struct ProcessNode
		{
			State state = State::READY;
			std::size_t in_degree = 0;
			std::size_t mutex_degree = 0;
			Misc::IndexSpan<> direct_edges;
			Misc::IndexSpan<> mutex_edges;
			std::size_t init_in_degree = 0;
			Task::Node::Ptr node;
			Task::Property property;
			std::size_t pause_count = 0;
			bool need_append_mutex = false;
		};

		bool TryStartupNode_AssumedLock(Flow& flow, Task::ContextWrapper& context, ProcessNode& node, std::size_t index);
		bool FinishNode_AssumedLock(Flow& flow, Task::ContextWrapper& context, ProcessNode& node, std::size_t index);
		bool Pause_AssumedLock(ProcessNode& node);
		bool Continue_AssumedLock(ProcessNode& node);

		Config config;

		std::mutex process_mutex;
		std::size_t version = 0;
		State current_state = State::READY;
		std::pmr::vector<ProcessNode> process_nodes;
		std::pmr::vector<std::size_t> process_edges;

		struct AppendEdge
		{
			std::size_t from;
			std::size_t to;
		};

		std::pmr::vector<AppendEdge> append_direct_edge;

		std::size_t finished_task = 0;
		std::size_t request_task = 0;
		std::size_t temporary_node_offset = 0;
		std::size_t temporary_edge_offset = 0;

		Task::TriggerProperty trigger;

		virtual void AddTaskGraphicFlowProcessContextRef() const = 0;
		virtual void SubTaskGraphicFlowProcessContextRef() const = 0;
		void AddNodeDataRef() const override { AddTaskGraphicFlowProcessContextRef(); }
		void SubNodeDataRef() const override { SubTaskGraphicFlowProcessContextRef(); }

		friend struct Flow;
	};

	export struct Flow : protected Task::Node, public Task::Trigger
	{
		struct Wrapper
		{
			void AddRef(Flow const* ptr) { ptr->AddTaskGraphicFlowRef(); }
			void SubRef(Flow const* ptr) { ptr->SubTaskGraphicFlowRef(); }
			operator Task::Node::Wrapper() const { return {}; }
			operator Task::Trigger::Wrapper() const { return {}; }
		};

		using Ptr = Pointer::IntrusivePtr<Flow, Wrapper>;

		enum class EdgeType
		{
			Direct,
			//ReverseDirect,
			Mutex,
		};

		struct Config
		{
			std::pmr::memory_resource* self_resource = std::pmr::get_default_resource();
			std::pmr::memory_resource* temporary_resource = std::pmr::get_default_resource();
		};

		static Flow::Ptr CreateTaskFlow(Config config);

		GraphNode AddNode(Task::Node::Ptr node, Task::Property property = {})
		{
			std::lock_guard lg(preprocess_mutex);
			return AddNode_AssumedLocked(std::move(node), property);
		}

		template<Task::AcceptableTaskNode Func>
		GraphNode AddLambdaNode(Func&& func, Task::Property property = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			auto ptr = Task::Node::CreateLambdaNode(std::forward<Func>(func), resource);
			if (ptr)
			{
				return this->AddNode(std::move(ptr), property);
			}
			return {};
		}

		bool Remove(GraphNode node) { std::lock_guard lg(preprocess_mutex); return Remove_AssumedLocked(node); }
		bool AddDirectEdge(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {}) { std::lock_guard lg(preprocess_mutex); return AddDirectEdge_AssumedLocked(from, direct_to, optimize); }
		bool AddMutexEdge(GraphNode from, GraphNode direct_to, Graph::EdgeOptimize optimize = {}) { std::lock_guard lg(preprocess_mutex); return AddMutexEdge_AssumedLocked(from, direct_to, optimize); }
		bool RemoveDirectEdge(GraphNode from, GraphNode direct_to) { std::lock_guard lg(preprocess_mutex); return RemoveDirectEdge_AssumedLocked(from, direct_to); }

		virtual void TaskFlowExecuteBegin(Task::ContextWrapper& context) {}
		virtual void TaskFlowExecuteEnd(Task::ContextWrapper& context) {}

		~Flow();

		std::optional<std::span<GraphEdge const>> AcyclicEdgeCheck(std::span<GraphEdge> output, CheckOptimize optimize = {})
		{
			std::lock_guard lg(preprocess_mutex);
			return this->AcyclicEdgeCheck_AssumedLocked(output, optimize);
		}

		bool Commited(Task::Context& context, std::u8string_view display_name = u8"", Task::Catgegory catrgory = {});
		GraphNode AddFlow(Flow& flow, std::u8string_view display_name = u8"", Task::Catgegory catrgory = {});

		template<Task::AcceptableTaskNode Func>
		GraphNode AddLambda(Func&& func, Task::Property property, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			auto task = Task::Node::CreateLambdaNode(std::forward<Func>(func), resource);
			if (task)
			{
				return AddNode(std::move(task), std::move(property));
			}
			return {};
		}

		static bool PauseAndLaunch(Task::ContextWrapper& wrapper, Node& node, Task::Property propery = {}, std::optional<std::chrono::steady_clock::time_point> delay = std::nullopt);

	protected:

		//virtual bool MarkNodePause(std::size_t node_identity);
		//virtual bool ContinuePauseNode(TaskContext& context, std::size_t node_identity);

		virtual FlowProcessContext::Ptr CreateProcessContext();

		Flow(Config config = {});

		GraphNode AddNode_AssumedLocked(Task::Node::Ptr node, Task::Property property);

		std::optional<std::span<Graph::GraphEdge const>> AcyclicEdgeCheck_AssumedLocked(std::span<Graph::GraphEdge> output_span, Graph::CheckOptimize optimize = {})
		{
			auto re = graph.AcyclicCheck(output_span, optimize);
			if (re.has_value())
			{
				update_state = UpdateState::Bad;
				return re;
			}else
			{
				update_state = UpdateState::None;
				++version;
				return std::nullopt;
			}
		}

		bool Remove_AssumedLocked(GraphNode node);
		bool AddDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {});
		bool AddMutexEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {});
		bool RemoveDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to);

		virtual void TaskExecute(Task::ContextWrapper& wrapper) override;
		virtual void TaskTerminal(Task::ContextWrapper& wrapper) noexcept override;
		virtual void TriggerExecute(Task::ContextWrapper& wrapper) override;
		virtual void TriggerTerminal(Task::ContextWrapper& wrapper) noexcept override;

		virtual void AddTaskGraphicFlowRef() const = 0;
		virtual void SubTaskGraphicFlowRef() const = 0;
		virtual void AddTaskNodeRef() const override { AddTaskGraphicFlowRef(); }
		virtual void SubTaskNodeRef() const override { SubTaskGraphicFlowRef(); }
		virtual void AddTriggerRef() const override { AddTaskGraphicFlowRef(); }
		virtual void SubTriggerRef() const override { SubTaskGraphicFlowRef(); }
		//virtual void AddNodeDataRef() const override { AddTaskGraphicFlowRef(); }
		//virtual void SubNodeDataRef() const override { SubTaskGraphicFlowRef(); }

		struct PreprocessEdge
		{
			std::size_t from;
			std::size_t to;

			bool operator==(PreprocessEdge const&) const = default;
		};

		struct PreprocessNode
		{
			Task::Node::Ptr node;
			Task::Property property;
			GraphNode self;
		};

		Config config;

		std::shared_mutex preprocess_mutex;
		Graph::DirectedAcyclicGraphDefer graph;
		std::pmr::vector<PreprocessNode> preprocess_nodes;
		std::pmr::vector<PreprocessEdge> preprocess_mutex_edges;
		std::size_t version = 0;

		enum class UpdateState
		{
			None,
			Updated,
			Bad,
		};

		UpdateState update_state = UpdateState::None;

		friend struct FlowProcessContext;
	};


	/*
	using Graph::GraphNode;
	using Graph::GraphEdge;
	using Graph::EdgeOptimize;
	using Graph::CheckOptimize;

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

		virtual bool Update(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) { return true; }

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) = 0;
		virtual void TaskFlowNodeTerminal(TaskProperty property) noexcept {}
		virtual ~TaskFlowNode() = default;

	protected:

		TaskFlowNode() = default;


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

		GraphNode AddNode(TaskFlowNode::Ptr node, TaskFlowNodeProperty property, std::size_t append_info = 0)
		{
			std::lock_guard lg(preprocess_mutex);
			return AddNode_AssumedLocked(std::move(node), property, append_info);
		}

		template<typename Func>
		GraphNode AddLambda(Func&& func, TaskFlowNodeProperty property, std::size_t append_info = 0, std::pmr::memory_resource *resource = std::pmr::get_default_resource()) requires(std::is_invocable_v<Func, TaskFlowContext&>)
		{
			auto ptr = CreateLambdaTask(std::forward<Func>(func), resource);
			if(ptr)
			{
				return this->AddNode(std::move(ptr), property, append_info);
			}
			return {};
		}

		bool Remove(GraphNode node) { std::lock_guard lg(preprocess_mutex); return Remove_AssumedLocked(node); }
		bool AddDirectEdge(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {}) { std::lock_guard lg(preprocess_mutex); return AddDirectEdge_AssumedLocked(from, direct_to , optimize); }
		bool AddMutexEdge(GraphNode from, GraphNode direct_to, Graph::EdgeOptimize optimize = {}) { std::lock_guard lg(preprocess_mutex); return AddMutexEdge_AssumedLocked(from, direct_to, optimize); }
		bool RemoveDirectEdge(GraphNode from, GraphNode direct_to) { std::lock_guard lg(preprocess_mutex); return RemoveDirectEdge_AssumedLocked(from, direct_to); }

		virtual void TaskFlowExecuteBegin(TaskFlowContext& context) {}
		virtual void TaskFlowExecuteEnd(TaskFlowContext& context) {}

		bool Update(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) override
		{
			std::lock_guard lg(process_mutex);
			return Update_AssumedLocked(resource);
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
		bool AddTemporaryNode(TaskFlowNode::Ptr node, TaskFlowNodeProperty property, Func&& func)
			requires(std::is_invocable_r_v<bool, Func, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex>)
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(std::move(node), property, std::forward<Func>(func));
		}

		bool AddTemporaryNode(TaskFlowNode::Ptr node, TaskFlowNodeProperty property)
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(std::move(node), property, nullptr, nullptr);
		}

		std::optional<std::span<GraphEdge const>> AcyclicEdgeCheck(std::span<GraphEdge> output, CheckOptimize optimize = {}, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource())
		{
			std::lock_guard lg(preprocess_mutex);
			return this->AcyclicEdgeCheck_AssumedLocked(output, optimize, temp_resource);
		}

	protected:
			
		TaskFlow(std::pmr::memory_resource* task_flow_resource = std::pmr::get_default_resource());

		GraphNode AddNode_AssumedLocked(TaskFlowNode::Ptr node, TaskFlowNodeProperty property, std::size_t append_info = 0);

		std::optional<std::span<Graph::GraphEdge const>> AcyclicEdgeCheck_AssumedLocked(std::span<Graph::GraphEdge> output_span, Graph::CheckOptimize optimize = {}, std::pmr::memory_resource * temp_resource = std::pmr::get_default_resource())
		{
			return graph.AcyclicCheck(output_span, optimize, temp_resource);
		}

		virtual bool Update_AssumedLocked(std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		bool Commited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property, std::chrono::steady_clock::time_point time_point);
		bool Commited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property);

		template<typename Func>
		bool AddTemporaryNode_AssumedLocked(TaskFlowNode::Ptr node, TaskFlowNodeProperty property, Func&& func)
			requires(std::is_invocable_r_v<bool, Func, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex>)
		{
			return this->AddTemporaryNode_AssumedLocked(node, property, [](void* append_data, TaskFlowNode const& node, TaskFlowNodeProperty pro, TemporaryNodeIndex index) -> bool
			{
				return (*static_cast<Func*>(append_data))(node, pro, index);
			}, &func);
		}

		bool AddTemporaryNode_AssumedLocked(TaskFlowNode::Ptr node, TaskFlowNodeProperty property, bool(*detect_func)(void* append_data, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex), void* append_data);
		bool SubTaskCommited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property);
		bool Remove_AssumedLocked(GraphNode node);
		bool AddDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {});
		bool AddMutexEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {});
		bool RemoveDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to);

		virtual void TaskFlowNodeExecute(TaskFlowContext& status) override;
		virtual void TaskExecute(TaskContextWrapper& status) override;

		virtual void AddTaskFlowRef() const = 0;
		virtual void SubTaskFlowRef() const = 0;
		virtual void AddTaskRef() const override { AddTaskFlowRef(); }
		virtual void SubTaskRef() const override { SubTaskFlowRef(); }
		void AddTaskFlowNodeRef() const override { AddTaskFlowRef(); }
		void SubTaskFlowNodeRef() const override { SubTaskFlowRef(); }

		struct PreprocessEdge
		{
			std::size_t from;
			std::size_t to;

			bool operator==(PreprocessEdge const&) const = default;
		};

		struct PreprocessNode
		{
			TaskFlowNode::Ptr node;
			TaskFlowNodeProperty property;
			GraphNode self;
		};

		std::mutex preprocess_mutex;
		Graph::DirectedAcyclicGraphDefer graph;
		std::pmr::vector<PreprocessNode> preprocess_nodes;
		std::pmr::vector<PreprocessEdge> preprocess_mutex_edges;
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
		std::size_t request_task = 0;
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
	*/

	

	
}

/*
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
*/