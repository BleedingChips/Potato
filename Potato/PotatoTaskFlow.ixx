module;

#include <cassert>

export module PotatoTaskFlow;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;
import PotatoTask;
import PotatoGraph;


export namespace Potato::TaskFlow
{

	using Task::TimeT;
	using Graph::GraphNode;
	using Graph::CheckOptimize;
	using Graph::EdgeOptimize;
	using Graph::GraphEdge;

	export struct Flow;
	export struct FlowExecutor;
	export struct FlowController;

	export struct Node
	{
		struct Wrapper
		{
			void AddRef(Node const* ptr) { ptr->AddTaskGraphicNodeRef(); }
			void SubRef(Node const* ptr) { ptr->SubTaskGraphicNodeRef(); }
			operator Task::Node::Wrapper() const { return {}; }
		};

		using Ptr = Pointer::IntrusivePtr<Node, Wrapper>;

		struct Parameter
		{
			std::u8string_view node_name;
			std::size_t acceptable_mask = std::numeric_limits<std::size_t>::max();
			Task::CustomData custom_data;
		};

		virtual void TaskFlowNodeExecute(Task::Context& context, FlowController& controller, Parameter& parameter) = 0;

	protected:

		virtual void AddTaskGraphicNodeRef() const = 0;
		virtual void SubTaskGraphicNodeRef() const = 0;
	};

	struct EncodedFlow
	{

		enum class Category
		{
			NormalNode,
			SubFlowBegin,
			SubFlowEnd
		};

		struct Info
		{
			Category category;
			Node::Ptr node;
			Node::Parameter parameter;
			Misc::IndexSpan<> direct_edges;
			Misc::IndexSpan<> mutex_edges;
			std::size_t in_degree = 0;
			Misc::IndexSpan<> owning_sub_flow;
		};

		EncodedFlow(std::pmr::memory_resource* resource) : encode_infos(resource), edges(resource) {}

		std::pmr::vector<Info> encode_infos;
		std::pmr::vector<std::size_t> edges;
	};

	template<typename Type>
	concept AcceptableTaskFlowNode = std::is_invocable_v < Type, Task::Context&, FlowController&, Node::Parameter&> ;

	export struct Flow
	{

		struct NodeIndex
		{
			std::size_t index = std::numeric_limits<std::size_t>::max();
			std::size_t version = 0;
			std::strong_ordering operator<=>(const NodeIndex&) const = default;
		};

		NodeIndex AddNode(Node& node, Node::Parameter parameter = {});

		template<AcceptableTaskFlowNode FuncT>
		static Node::Ptr CreateNode(FuncT&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		template<AcceptableTaskFlowNode FuncT>
		NodeIndex AddNode(FuncT&& func, Node::Parameter parameter = {}, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		{
			auto node = CreateNode(std::forward<FuncT>(func), resource);
			if (node)
			{
				return AddNode(*node, parameter);
			}
			return {};
		}

		NodeIndex AddFlowAsNode(Flow const& flow, Node::Ptr sub_flow_node = {}, Node::Parameter parameter = {}, std::pmr::memory_resource* temporary_resource = std::pmr::get_default_resource());
		bool Remove(NodeIndex const& index);
		bool AddDirectEdge(NodeIndex from, NodeIndex direct_to);
		bool AddMutexEdge(NodeIndex from, NodeIndex direct_to);
		bool RemoveDirectEdge(NodeIndex from, NodeIndex direct_to);
		bool RemoveMutexEdge(NodeIndex from, NodeIndex direct_to);
		Flow(std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		bool IsAvailableIndex(NodeIndex const& index) const;

	protected:

		enum class NodeCategory
		{
			Empty,
			NormalNode,
			SubFlowNode
		};

		struct NodeInfos
		{
			Node::Ptr node;
			Node::Parameter parameter;
			std::size_t version = 1;
			NodeCategory category = NodeCategory::Empty;
			Misc::IndexSpan<> encode_nodes;
			Misc::IndexSpan<> encode_edges;
		};

		static std::optional<std::size_t> EncodeNodeTo(
			Flow const& target_flow,
			EncodedFlow& output_encoded_flow,
			std::pmr::memory_resource* temporary_resource = std::pmr::get_default_resource()
			);

		std::pmr::vector<NodeInfos> node_infos;
		std::pmr::vector<Graph::GraphEdge> node_direct_edges;
		std::pmr::vector<Graph::GraphEdge> node_mutex_edges;

		EncodedFlow encoded_flow;

		friend struct FlowExecutor;
	};

	export struct FlowExecutor : protected Task::Node
	{

		struct Wrapper
		{
			void AddRef(FlowExecutor const* ptr) { ptr->AddTaskFlowExecutorRef(); }
			void SubRef(FlowExecutor const* ptr) { ptr->SubTaskFlowExecutorRef(); }
			operator Task::Node::Wrapper() const { return {}; }
		};

		using Ptr = Pointer::IntrusivePtr<FlowExecutor, Wrapper>;

		struct PauseMountPoint
		{
			~PauseMountPoint();
			PauseMountPoint(PauseMountPoint&& point);
			bool Continue(Task::Context& context);
			operator bool() const { return exectutor; }
		protected:
			PauseMountPoint(FlowExecutor::Ptr exectutor, std::size_t encoded_flow_index)
				: exectutor(std::move(exectutor)), encoded_flow_index(encoded_flow_index)
			{
			}
			PauseMountPoint() = default;
			FlowExecutor::Ptr exectutor;
			std::size_t encoded_flow_index = std::numeric_limits<std::size_t>::max();

			friend struct FlowExecutor;
		};

		static Ptr Create(std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		bool UpdateFromFlow(Flow const& target_flow, std::pmr::memory_resource* temporary_resource = std::pmr::get_default_resource());
		bool UpdateState();
		bool Commit(Task::Context& context, Task::Node::Parameter flow_parameter = {});


		struct TemplateSequencer
		{

		};

	protected:

		virtual void BeginFlow(Task::Context& context, Task::Node::Parameter parameter) {};
		virtual void EndFlow(Task::Context& context, Task::Node::Parameter parameter) {};
		virtual void TaskExecute(Task::Context& context, Parameter& parameter) override;
		virtual void TaskTerminal(Parameter& parameter) noexcept override;
		virtual void AddTaskNodeRef() const override;
		virtual void SubTaskNodeRef() const override;
		PauseMountPoint CreatePauseMountPoint(std::size_t encoded_flow_index);
		bool ContinuePauseMountPoint(Task::Context& context, std::size_t encoded_flow_index);
		bool TerminalPauseMountPoint(std::size_t encoded_flow_index);
		virtual void AddTaskFlowExecutorRef() const = 0;
		virtual void SubTaskFlowExecutorRef() const = 0;
		bool AddTemplateNode(TaskFlow::Node& target_node, TaskFlow::Node::Parameter parameter, bool (*func)(void* data, TemplateSequencer& sequencer, std::size_t), void* append_data, std::size_t startup_index, std::pmr::memory_resource* resource);

		FlowExecutor(std::pmr::memory_resource* resource);

		void TryStartupNode_AssumedLocked(Task::Context& context, std::size_t encoded_flow_index);
		void FinishNode_AssumedLocked(Task::Context& context, std::size_t encoded_flow_index);
		std::shared_mutex encoded_flow_mutex;
		EncodedFlow encoded_flow;
		std::size_t encoded_flow_out_degree = 0;

		struct ExecuteState
		{
			enum class State : std::uint8_t
			{
				Ready,
				Running,
				Pause,
				Done,
				PauseTerminal,
				FlowTerminal,
			};
			
			std::size_t in_degree = 0;
			std::size_t mutex_degree = 0;
			std::size_t pause_count = 0;
			State state : 7 = State::Ready;
			std::uint8_t has_template_edges : 1 = false;
		};

		struct TemplateNode
		{
			TaskFlow::Node::Ptr node;
			TaskFlow::Node::Parameter parameter;
			bool has_template_edge = false;
		};

		struct TemplateEdge
		{
			std::size_t from = 0;
			std::size_t to = 0;
		};

		std::shared_mutex template_node_mutex;
		std::pmr::vector<TemplateNode> template_node;
		std::pmr::vector<TemplateEdge> template_edges;
		std::size_t encoded_flow_node_count_for_template = 0;

		std::mutex execute_state_mutex;
		Task::Node::Parameter executor_parameter;
		ExecuteState::State execute_state = ExecuteState::State::Ready;
		std::pmr::vector<ExecuteState> encoded_flow_execute_state;
		std::size_t execute_out_degree = 0;
		std::size_t encoded_flow_node_count_for_execute = 0;

		friend struct FlowController;
	};

	export struct FlowController
	{

		EncodedFlow::Category GetCategory() const { return category; }
		FlowExecutor::PauseMountPoint MarkCurrentAsPause();

	protected:

		FlowController(FlowExecutor& exe, std::size_t encoded_flow_index)
			: executor(exe), encoded_flow_index(encoded_flow_index)
		{
			
		}
		FlowExecutor& executor;
		std::size_t encoded_flow_index;
		EncodedFlow::Category category = EncodedFlow::Category::NormalNode;

		friend struct FlowExecutor;
	};

}

namespace Potato::TaskFlow
{
	template<AcceptableTaskFlowNode TaskFlowNodeT>
	struct TaskFlowNodeLambdaWrapper : public Node, public IR::MemoryResourceRecordIntrusiveInterface
	{

	protected:

		TaskFlowNodeLambdaWrapper(IR::MemoryResourceRecord record, TaskFlowNodeT&& node)
			: MemoryResourceRecordIntrusiveInterface(record),  task_flow_node(std::forward<TaskFlowNodeT>(node))
		{
			
		}

		void AddTaskGraphicNodeRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		void SubTaskGraphicNodeRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }
		void TaskFlowNodeExecute(Task::Context& context, FlowController& controller, Parameter& parameter) override
		{
			task_flow_node(context, controller, parameter);
		}

		TaskFlowNodeT task_flow_node;

		friend struct Flow;
	};

	template<AcceptableTaskFlowNode FuncT>
	Node::Ptr Flow::CreateNode(FuncT&& func, std::pmr::memory_resource* resource)
	{
		using Type = TaskFlowNodeLambdaWrapper<std::remove_cvref_t<FuncT>>;
		auto Record = Potato::IR::MemoryResourceRecord::Allocate<Type>(resource);
		if (Record)
		{
			return new (Record.Get()) Type{ Record, std::forward<FuncT>(func) };
		}
		return {};
	}
}