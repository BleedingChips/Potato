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

	using Task::TimeT;
	using Graph::GraphNode;
	using Graph::CheckOptimize;
	using Graph::EdgeOptimize;
	using Graph::GraphEdge;

	export struct Flow;

	export struct Node;

	export struct Node
	{
		struct Wrapper
		{
			void AddRef(Node const* ptr) { ptr->AddTaskGraphicNodeRef(); }
			void SubRef(Node const* ptr) { ptr->SubTaskGraphicNodeRef(); }
			operator Task::Node::Wrapper() const { return {}; }
		};

		using Ptr = Pointer::IntrusivePtr<Node, Wrapper>;

		using Parameter = Task::Node::Parameter;

		virtual void TaskGraphicNodeExecute(Task::Context& context, Node& self, Parameter& parameter) = 0;
		virtual void TaskGraphicNodeTerminal(Node& self, Parameter& parameter) noexcept {}

	protected:

		virtual void AddTaskGraphicNodeRef() const = 0;
		virtual void SubTaskGraphicNodeRef() const = 0;
	};

	export struct Flow
	{
		GraphNode AddNode(Node& node, Node::Parameter parameter = {});
		GraphNode AddFlowAsNode(Flow const& flow);
		bool Remove(GraphNode node);
		bool AddDirectEdge(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {});
		bool AddMutexEdge(GraphNode from, GraphNode direct_to, Graph::EdgeOptimize optimize = {});
		bool RemoveDirectEdge(GraphNode from, GraphNode direct_to);
		Flow(std::pmr::memory_resource* resource = std::pmr::get_default_resource());

	protected:

		Graph::DirectedAcyclicGraphDefer graph;

		struct NodeInfos
		{
			Node::Ptr node;
			Node::Parameter parameter;
		};

		struct NodeMutexEdge
		{
			std::size_t from;
			std::size_t to;
		};

		struct SubNodeInfos
		{
			Node::Ptr node;
			Node::Parameter parameter;
			std::size_t sub_flow_index;
		};

		std::pmr::vector<NodeInfos> node_infos;
		std::pmr::vector<NodeMutexEdge> node_mutex_edges;
		std::pmr::vector<SubNodeInfos> node_infos;
	};




	/*
	export struct Node : protected Task::Node
	{
		struct Wrapper
		{
			void AddRef(Node const* ptr) { ptr->AddTaskGraphicNodeRef(); }
			void SubRef(Node const* ptr) { ptr->SubTaskGraphicNodeRef(); }
			operator Task::Node::Wrapper() const { return {}; }
		};

		using Ptr = Pointer::IntrusivePtr<Node, Wrapper>;

		virtual void TaskGraphicNodeExecute(Flow& wrapper, Task::Context& context, Task::Property property) = 0;
		virtual void TaskGraphicNodeTerminal(Task::Property property) noexcept {}

		virtual bool AcceptFlow(Flow const& flow) { return true; };
		virtual void OnLeaveFlow(Flow const& flow) {}

	protected:

		void TaskExecute(Task::Context& context, Task::Node& self, Task::Property property, Task::TriggerProperty& trigger_property) override;
		void TaskTerminal(Task::ContextWrapper& property) noexcept override;

		virtual void AddTaskGraphicNodeRef() const = 0;
		virtual void SubTaskGraphicNodeRef() const = 0;
		void AddTaskNodeRef() const override final { AddTaskGraphicNodeRef(); }
		void SubTaskNodeRef() const override final { SubTaskGraphicNodeRef(); }
	};

	template<typename Type>
	concept AcceptableTaskGraphicNode = std::is_invocable_v<Type, ContextWrapper&>;

	struct FlowNodeDetectionIndex
	{
		std::size_t current_index;
		std::size_t process_node_count;
	};

	template<typename Type>
	concept AcceptableTemporaryDetectFunction = std::is_invocable_r_v<bool, Type, Node&, Task::Property&, Node const&, Task::Property const&, FlowNodeDetectionIndex const&>;

	export struct Flow : protected Node, public Task::Trigger
	{
		struct Wrapper : public Node::Wrapper, public Task::Trigger::Wrapper
		{
			void AddRef(Flow const* ptr) { ptr->AddTaskGraphicFlowRef(); }
			void SubRef(Flow const* ptr) { ptr->SubTaskGraphicFlowRef(); }
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

		template<AcceptableTaskGraphicNode FunT>
		static Ptr CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		static Flow::Ptr CreateTaskFlow(Config config);

		template<AcceptableTaskGraphicNode Func>
		GraphNode AddLambda(Func&& func, Task::Property property = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		GraphNode AddNode(Node& node, Task::Property property = {});
		GraphNode AddNode(Flow& node, Task::Property property = {}) { return AddNode(static_cast<Node&>(node), std::move(property)); }
		template<AcceptableTemporaryDetectFunction Func>
		bool AddTemporaryNode(Node& node, Func&& func, Task::Property property = {});
		bool AddTemporaryNode(Node& node, Task::Property property = {});


		bool Remove(GraphNode node);
		bool AddDirectEdge(GraphNode from, GraphNode direct_to, EdgeOptimize optimize= {});
		bool AddMutexEdge(GraphNode from, GraphNode direct_to, Graph::EdgeOptimize optimize = {});
		bool RemoveDirectEdge(GraphNode from, GraphNode direct_to);

		virtual void TaskFlowExecuteBegin_AssumedLocked(Task::ContextWrapper& context) {}
		virtual void TaskFlowExecuteEnd_AssumedLocked(Task::ContextWrapper& context) {}
		virtual void TaskFlowPostUpdateProcessNode_AssumedLocked(Task::ContextWrapper& context) {}

		~Flow();

		std::optional<std::span<GraphEdge const>> AcyclicEdgeCheck(std::span<GraphEdge> output, CheckOptimize optimize = {});

		bool Commited(Task::Context& context, Task::Property property = {}, Task::TriggerProperty trigger_property = {}, std::optional<TimeT::time_point> delay = std::nullopt);
		bool Commited(Task::ContextWrapper& context, Task::Property property = {}, Task::TriggerProperty trigger_property = {}, std::optional<TimeT::time_point> delay = std::nullopt);


		bool PauseAndLaunch(ContextWrapper& wrapper, Node& node, Task::Property property = {}, std::optional<TimeT::time_point> delay = std::nullopt);
		bool PauseAndLaunch(ContextWrapper& wrapper, Task::Node& node, Task::Property property = {}, std::optional<TimeT::time_point> delay = std::nullopt);

	protected:

		Flow(Config config = {});

		bool Commited_AssumedLocked(Task::Context& context, Task::Property property = {}, Task::TriggerProperty trigger_property = {}, std::optional<TimeT::time_point> delay = std::nullopt);
		bool Commited_AssumedLocked(Task::ContextWrapper& context, Task::Property property = {}, Task::TriggerProperty trigger_property = {}, std::optional<TimeT::time_point> delay = std::nullopt);


		void OnLeaveFlow(Flow const& flow) override;
		bool AcceptFlow(Flow const& flow) override;
		GraphNode AddNode_AssumedLocked(Node& node, Task::Property property);
		bool AddTemporaryNode_AssumedLocked(Node& node, Task::Property property, bool(*detect_func)(void* append_data, Node& temp_node, Task::Property& temp_property, Node const&, Task::Property const&, FlowNodeDetectionIndex const& index), void* append_data);
		template<AcceptableTemporaryDetectFunction Func>
		bool AddTemporaryNode_AssumedLocked(Node& node, Func&& func, Task::Property property = {});


		std::optional<std::span<Graph::GraphEdge const>> AcyclicEdgeCheck_AssumedLocked(std::span<Graph::GraphEdge> output_span, Graph::CheckOptimize optimize = {});

		bool Remove_AssumedLocked(GraphNode node);
		bool AddDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {});
		bool AddMutexEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize = {});
		bool RemoveDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to);
		bool RemoveMutexEdge_AssumedLocked(GraphNode from, GraphNode direct_to);

		//virtual void PostUpdateFromFlow(FlowProcessContext& context, Task::ContextWrapper& wrapper) {};

		virtual void TaskGraphicNodeExecute(ContextWrapper& wrapper) override final {};
		virtual void TaskExecute(Task::ContextWrapper& wrapper) override final;
		virtual void TaskTerminal(Task::ContextWrapper& wrapper) noexcept override final;
		virtual void TriggerExecute(Task::ContextWrapper& wrapper) override final;
		virtual void TriggerTerminal(Task::ContextWrapper& wrapper) noexcept override final;

		virtual void AddTaskGraphicFlowRef() const = 0;
		virtual void SubTaskGraphicFlowRef() const = 0;
		void AddTriggerRef() const override final { AddTaskGraphicFlowRef(); }
		void SubTriggerRef() const override final { SubTaskGraphicFlowRef(); }
		void AddTaskGraphicNodeRef() const override final { AddTaskGraphicFlowRef(); }
		void SubTaskGraphicNodeRef() const override final { SubTaskGraphicFlowRef(); }

		struct PreprocessEdge
		{
			std::size_t from;
			std::size_t to;

			bool operator==(PreprocessEdge const&) const = default;
		};

		struct PreprocessNode
		{
			Node::Ptr node;
			Task::Property property;
			GraphNode self;
			bool under_process = false;
		};

		Config config;

		enum class RunningState
		{
			Free,
			Ready,
			Running,
			Done,


			Locked,
			LockedRunning,
			LockedDone

		};

		std::shared_mutex preprocess_mutex;
		Graph::DirectedAcyclicGraphDefer graph;
		std::pmr::vector<PreprocessNode> preprocess_nodes;
		std::pmr::vector<PreprocessEdge> preprocess_mutex_edges;
		bool need_update = false;
		RunningState running_state = RunningState::Free;

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
			Node::Ptr node;
			Task::Property property;
			std::size_t pause_count = 0;
			bool need_append_mutex = false;
			GraphNode index;
		};

		bool TryStartupNode_AssumedLock(Task::ContextWrapper& context, ProcessNode& node, std::size_t index);
		bool FinishNode_AssumedLock(Task::ContextWrapper& context, ProcessNode& node, std::size_t index);
		bool Pause_AssumedLock(ProcessNode& node);
		bool Continue_AssumedLock(ProcessNode& node);
		void UpdateProcessNode();
		virtual bool UpdateProcessNode_AssumedLocked();
		bool Start(Task::ContextWrapper& wrapper);
		std::mutex process_mutex;
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
		Task::Property task_property;

		friend struct FlowProcessContext;
	};


	template<AcceptableTaskGraphicNode Func>
	GraphNode Flow::AddLambda(Func&& func, Task::Property property, std::pmr::memory_resource* resource)
	{
		auto ptr = Node::CreateLambdaNode(std::forward<Func>(func), resource);
		if (ptr)
		{
			return this->AddNode(*ptr, property);
		}
		return {};
	}

	template<AcceptableTemporaryDetectFunction Func>
	bool Flow::AddTemporaryNode_AssumedLocked(Node& node, Func&& func, Task::Property property)
	{
		return this->AddTemporaryNode_AssumedLocked(node, std::move(property), [](void* append_data, Node& temp_node, Task::Property& temp_property, Node const& target_node, Task::Property const& target_property, FlowNodeDetectionIndex const& index) -> bool
			{
				return (*static_cast<Func*>(append_data))(temp_node, temp_property, target_node, target_property, index);
			}, &func);
	}

	template<AcceptableTemporaryDetectFunction Func>
	bool Flow::AddTemporaryNode(Node& node, Func&& func, Task::Property property)
	{
		std::lock_guard lg(process_mutex);
		return this->AddTemporaryNode_AssumedLocked(node, std::forward<Func>(func), std::move(property));
	}

	template<typename LanbdaT>
	struct LambdaTaskGraphicNode : public Node, public Potato::IR::MemoryResourceRecordIntrusiveInterface
	{


		template<typename FunT>
		LambdaTaskGraphicNode(Potato::IR::MemoryResourceRecord Record, FunT&& Fun)
			: MemoryResourceRecordIntrusiveInterface(Record), TaskInstance(std::forward<FunT>(Fun))
		{

		}

		virtual void TaskGraphicNodeExecute(ContextWrapper& Status) override
		{
			TaskInstance.operator()(Status);
		}

	protected:

		virtual void AddTaskGraphicNodeRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubTaskGraphicNodeRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }

	private:

		LanbdaT TaskInstance;
	};

	template<AcceptableTaskGraphicNode FunT>
	Node::Ptr Node::CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource)
	{
		using Type = LambdaTaskGraphicNode<std::remove_cvref_t<FunT>>;
		auto Record = Potato::IR::MemoryResourceRecord::Allocate<Type>(resource);
		if (Record)
		{
			return new (Record.Get()) Type{ Record, std::forward<FunT>(func) };
		}
		return {};
	}
	*/
}