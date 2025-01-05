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

	struct NodeData
	{
		struct Wrapper
		{
			void AddRef(NodeData const* ptr) { ptr->AddNodeDataRef(); }
			void SubRef(NodeData const* ptr) { ptr->SubNodeDataRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<NodeData, Wrapper>;
		virtual ~NodeData() = default;
	protected:

		virtual void AddNodeDataRef() const = 0;
		virtual void SubNodeDataRef() const = 0;
	};

	export struct ContextWrapper;
	export struct Node;

	struct Trigger
	{
		struct Wrapper
		{
			void AddRef(Trigger const* ptr) { ptr->AddTriggerRef(); }
			void SubRef(Trigger const* ptr) { ptr->SubTriggerRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<Trigger, Wrapper>;

		virtual void TriggerExecute(ContextWrapper& wrapper) = 0;
		virtual void TriggerTerminal(ContextWrapper& wrapper) noexcept = 0;

		virtual ~Trigger() = default;
	protected:

		virtual void AddTriggerRef() const = 0;
		virtual void SubTriggerRef() const = 0;
	};

	enum class Priority : std::size_t
	{
		High = 0,
		Normal = 1,
		Low = 2,

		Max
	};

	struct Category
	{

		template<typename Type>
		std::optional<Type> GetOptional() const requires(TMP::IsOneOfV<std::remove_cvref_t<Type>, std::monostate, std::size_t, std::thread::id>)
		{
			using T = std::remove_cvref_t<Type>;
			if (std::holds_alternative<Type>(category))
			{
				return std::get<Type>(category);
			}
			return std::nullopt;
		}


		bool IsGlobal() const { return std::holds_alternative<std::monostate>(category); }
		std::optional<std::size_t> GetGroupID() const { return GetOptional<std::size_t>(); }
		std::optional<std::thread::id> GetThreadID() const { return GetOptional<std::thread::id>(); }
	protected:
		std::variant<std::monostate, std::size_t, std::thread::id> category;
	};

	struct Property
	{
		std::u8string_view node_name;
		Category category;
		//Priority priority = Priority::Normal;
		
		

		struct Data
		{
			operator bool() const { return !std::holds_alternative<std::monostate>(data); }
			bool HasSizeT() const { return std::holds_alternative<std::size_t>(data); }
			std::size_t GetSizeT() const { return std::get<std::size_t>(data); }
			bool HasNodeDataPointer() const { return std::holds_alternative<NodeData::Ptr>(data); }
			NodeData::Ptr GetNodeDataPointer() const { return std::get<NodeData::Ptr>(data); }
			template<typename PointerType>
			PointerType* TryGetNodeDataPointerWithType() const
			{
				if (HasNodeDataPointer())
				{
					return dynamic_cast<PointerType*>(std::get<NodeData::Ptr>(data).GetPointer());
				}
				return nullptr;
			}
			Data(Data const&) = default;
			Data(Data&&) = default;
			Data() = default;
			Data& operator=(Data const&) = default;
			Data& operator=(Data&&) = default;
			Data& SetIndex(std::size_t index) { data = index; return *this; }
			Data& SetNodeData(NodeData::Ptr node_data) { data = std::move(node_data); return *this; }
		protected:
			std::variant<std::monostate, std::size_t, void*, NodeData::Ptr> data;
		};

		Data data;
		Data data2;
	};

	struct TriggerProperty
	{
		Trigger::Ptr trigger;
		Property::Data data;
		Property::Data data2;
	};

	enum class Status : std::size_t
	{
		Normal,
		GroupTerminal,
		ThreadTerminal,
		ContextTerminal,
	};

	export struct ContextWrapper
	{
		virtual Status GetStatue() const = 0;
		virtual std::size_t GetGroupId() const = 0;
		virtual Property& GetTaskNodeProperty() const = 0;
		virtual TriggerProperty& GetTriggerProperty() const = 0;
		virtual bool Commit(Node& target, Property property, TriggerProperty trigger = {}, std::optional<std::chrono::steady_clock::time_point> delay_time = std::nullopt) = 0;
		virtual Node& GetCurrentTaskNode() const = 0;
	};

	template<typename Type>
	concept AcceptableTaskNode = std::is_invocable_v<Type, ContextWrapper&>;

	export struct Node
	{

		struct Wrapper
		{
			void AddRef(Node const* ptr) { ptr->AddTaskNodeRef(); }
			void SubRef(Node const* ptr) { ptr->SubTaskNodeRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<Node, Wrapper>;

		template<AcceptableTaskNode FunT>
		static Ptr CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		virtual void TaskExecute(ContextWrapper& wrapper) = 0;
		virtual void TaskTerminal(ContextWrapper& wrapper) noexcept {};
		virtual ~Node() = default;

	protected:
		
		virtual void AddTaskNodeRef() const = 0;
		virtual void SubTaskNodeRef() const = 0;
	};

	struct NodeSequencer : public IR::MemoryResourceRecordIntrusiveInterface
	{

		using Ptr = Potato::Pointer::IntrusivePtr<NodeSequencer>;

		static auto Create(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) -> Ptr;

		struct NodeTuple
		{
			Node::Ptr node;
			Property property;
			TriggerProperty trigger;
		};

		struct TimedNodeTuple
		{
			NodeTuple node_tuple;
			TimeT::time_point request_time;
		};

		std::optional<NodeTuple> PopNode(TimeT::time_point current_time);
		std::optional<NodeTuple> ForcePopNode();

		bool InsertNode(NodeTuple node, std::optional<TimeT::time_point> delay_time);
		std::size_t GetTaskCount() const { return task_count; }
		void MarkNodeFinish() { task_count -= 1; }

	protected:

		NodeSequencer(Potato::IR::MemoryResourceRecord record, std::pmr::memory_resource* resource);

		std::atomic_size_t task_count = 0;

		std::mutex node_deque_mutex;
		std::optional<TimeT::time_point> min_time_point;
		std::pmr::vector<TimedNodeTuple> delay_node;
		std::pmr::deque<NodeTuple> node_deque;
	};


	export struct Context
	{
		static std::size_t GetSuggestThreadCount() { return std::thread::hardware_concurrency(); }

		std::size_t AddGroupThread(std::size_t group_id, std::size_t thread_count = 1, std::pmr::memory_resource* node_sequence_resource = std::pmr::get_default_resource());
		std::optional<std::thread::id> GetRandomThreadIDFromGroup(std::size_t group_id, std::size_t random = std::chrono::steady_clock::now().time_since_epoch().count());
		std::optional<std::thread::id> GetRandomThreadID(std::size_t random = std::chrono::steady_clock::now().time_since_epoch().count());

		struct Config
		{
			std::pmr::memory_resource* self_resource = std::pmr::get_default_resource();
			std::pmr::memory_resource* node_sequence_resource = std::pmr::get_default_resource();
		};

		Context(Config config = {});
		~Context();

		struct ThreadExecuteContext
		{
			std::size_t node_sequencer_selector = 0;
			std::size_t continuous_empty_node_sequencer_count = 0;
			std::size_t reached_node_sequencer_count = 0;
			std::size_t current_node_sequencer_iterator = 0;
		};

		bool ExecuteContextThreadOnce(ThreadExecuteContext& execute_context, TimeT::time_point now = TimeT::now(), bool accept_global = true, std::size_t group_id = std::numeric_limits<std::size_t>::max());

		bool CheckNodeSequencerEmpty();

		void ExecuteContextThreadUntilNoExistTask(std::size_t group_id = std::numeric_limits<std::size_t>::max());
		bool Commit(Node& target, Property property = {}, TriggerProperty trigger = {}, std::optional<TimeT::time_point> delay_time_point = std::nullopt);

	protected:

		NodeSequencer::Ptr node_sequencer_global;
		NodeSequencer::Ptr node_sequencer_context;

		struct ThreadGroupInfo
		{
			NodeSequencer::Ptr node_sequencer;
			std::size_t thread_count;
			std::size_t group_id;
		};

		struct ThreadInfo
		{
			NodeSequencer::Ptr node_sequencer;
			std::jthread thread;
			std::thread::id thread_id;
			std::size_t group_id;
		};

		std::shared_mutex infos_mutex;
		std::pmr::vector<ThreadGroupInfo> thread_group_infos;
		std::pmr::vector<ThreadInfo> thread_infos;

	private:

		void ThreadExecute(NodeSequencer& group, NodeSequencer& thread, std::size_t group_id, std::stop_token& sk);
		void TerminalNodeSequencer(Status state, NodeSequencer& target_sequence, std::size_t group_id) noexcept;
		bool ExecuteNodeSequencer(NodeSequencer& target_sequence, std::size_t group_id, TimeT::time_point now_time);
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

		virtual void TaskExecute(ContextWrapper& Status) override
		{
			TaskInstance.operator()(Status);
		}

	protected:

		virtual void AddTaskNodeRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubTaskNodeRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }

	private:

		LanbdaT TaskInstance;
	};

	template<AcceptableTaskNode FunT>
	auto Node::CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource)->Ptr
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