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
		void AddTemporaryNode(Task::Node& node, Func&& func, Task::Property property = {})
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(node, std::forward<Func>(func), std::move(property));
		}

		void AddTemporaryNode(Task::Node& node, Task::Property property = {})
		{
			std::lock_guard lg(process_mutex);
			return this->AddTemporaryNode_AssumedLocked(node, std::move(property), nullptr, nullptr);
		}

		struct Config
		{
			std::pmr::memory_resource* self_resource = std::pmr::get_default_resource();
			std::pmr::memory_resource* temporary_update_resource = std::pmr::get_default_resource();
		};

		~FlowProcessContext();
		Task::TriggerProperty const& GetTriggerProperty() const { return trigger; }
		void AddTemporaryNode_AssumedLocked(Task::Node& node, Task::Property property, bool(*detect_func)(void* append_data, Task::Node const&, Task::Property const&, FlowNodeDetectionIndex const& index), void* append_data);
		template<AcceptableTemporaryDetectFunction Func>
		void AddTemporaryNode_AssumedLocked(Task::Node& node, Func&& func, Task::Property property = {})
		{
			return this->AddTemporaryNode_AssumedLocked(node, std::move(property), [](void* append_data, Task::Node const& node, Task::Property const& property, FlowNodeDetectionIndex const& index) -> bool
				{
					return (*static_cast<Func*>(append_data))(node, property, index);
				}, &func);
		}
	protected:

		FlowProcessContext(Config config = {});
		bool UpdateFromFlow_AssumedLocked(Flow& flow);
		

		bool Start(Flow& flow, Task::ContextWrapper& wrapper);
		void OnTaskFlowTerminal(Flow& flow, Task::ContextWrapper& wrapper) noexcept {}
		bool OnTaskFlowFlowTriggerExecute(Flow& flow, Task::ContextWrapper& wrapper);
		bool OnTaskFlowFlowTriggerTerminal(Flow& flow, Task::ContextWrapper& wrapper) noexcept { return true; }
		bool PauseAndLaunch(Task::ContextWrapper& wrapper, Flow& flow, std::size_t index, Task::Node& ptr, Task::Property property, std::optional<std::chrono::steady_clock::time_point> delay_time);
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
		std::size_t real_request_task = 0;
		std::size_t temporary_node_offset = 0;
		std::size_t temporary_edge_offset = 0;

		Task::TriggerProperty trigger;

		virtual void AddTaskGraphicFlowProcessContextRef() const = 0;
		virtual void SubTaskGraphicFlowProcessContextRef() const = 0;
		void AddNodeDataRef() const final { AddTaskGraphicFlowProcessContextRef(); }
		void SubNodeDataRef() const final { SubTaskGraphicFlowProcessContextRef(); }

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

		GraphNode AddNode(Task::Node& node, Task::Property property = {})
		{
			std::lock_guard lg(preprocess_mutex);
			return AddNode_AssumedLocked(node, std::move(property));
		}

		template<Task::AcceptableTaskNode Func>
		GraphNode AddLambdaNode(Func&& func, Task::Property property = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			auto ptr = Task::Node::CreateLambdaNode(std::forward<Func>(func), resource);
			if (ptr)
			{
				return this->AddNode(*ptr, property);
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

		bool Commited(Task::Context& context, std::u8string_view display_name = u8"", Task::Category category = {});
		GraphNode AddFlow(Flow& flow, std::u8string_view display_name = u8"", Task::Category category = {});

		template<Task::AcceptableTaskNode Func>
		GraphNode AddLambda(Func&& func, Task::Property property, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			auto task = Task::Node::CreateLambdaNode(std::forward<Func>(func), resource);
			if (task)
			{
				return AddNode(*task, std::move(property));
			}
			return {};
		}

		static bool PauseAndLaunch(Task::ContextWrapper& wrapper, Node& node, Task::Property property = {}, std::optional<std::chrono::steady_clock::time_point> delay = std::nullopt);

		template<AcceptableTemporaryDetectFunction Func>
		static bool AddTemporaryNode(Task::ContextWrapper& wrapper, Node& node, Func&& func, Task::Property property = {})
		{
			auto [flow, context] = GetProcessContextFromWrapper(wrapper);
			if (context != nullptr)
			{
				context->AddTemporaryNode(node, func, std::move(property));
				return true;
			}
			return false;
		}

		static bool AddTemporaryNode(Task::ContextWrapper& wrapper, Node& node, Task::Property property = {});

	protected:

		static std::tuple<Flow*, FlowProcessContext*> GetProcessContextFromWrapper(Task::ContextWrapper& wrapper);

		//virtual bool MarkNodePause(std::size_t node_identity);
		//virtual bool ContinuePauseNode(TaskContext& context, std::size_t node_identity);

		virtual FlowProcessContext::Ptr CreateProcessContext() { std::lock_guard lg(preprocess_mutex); return CreateProcessContext_AssumedLocked(); }
		virtual FlowProcessContext::Ptr CreateProcessContext_AssumedLocked();

		Flow(Config config = {});

		GraphNode AddNode_AssumedLocked(Task::Node& node, Task::Property property);
		GraphNode AddFlow_AssumedLocked(Flow& flow, std::u8string_view display_name, Task::Category category);

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
		bool RemoveMutexEdge_AssumedLocked(GraphNode from, GraphNode direct_to);

		virtual void PostUpdateFromFlow(FlowProcessContext& context, Task::ContextWrapper& wrapper) {};

		virtual void TaskExecute(Task::ContextWrapper& wrapper) override;
		virtual void TaskTerminal(Task::ContextWrapper& wrapper) noexcept override;
		virtual void TriggerExecute(Task::ContextWrapper& wrapper) override;
		virtual void TriggerTerminal(Task::ContextWrapper& wrapper) noexcept override;

		virtual void AddTaskGraphicFlowRef() const = 0;
		virtual void SubTaskGraphicFlowRef() const = 0;
		virtual void AddTaskNodeRef() const final { AddTaskGraphicFlowRef(); }
		virtual void SubTaskNodeRef() const final { SubTaskGraphicFlowRef(); }
		virtual void AddTriggerRef() const final { AddTaskGraphicFlowRef(); }
		virtual void SubTriggerRef() const final { SubTaskGraphicFlowRef(); }

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
}