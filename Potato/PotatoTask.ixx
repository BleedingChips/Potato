module;

#include <cassert>

export module PotatoTask;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;


export namespace Potato::Task
{
	struct AppendInfo
	{
		struct Wrapper
		{
			void AddRef(AppendInfo const* ptr) { ptr->AddAppendInfoRef(); }
			void SubRef(AppendInfo const* ptr) { ptr->SubAppendInfoRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<AppendInfo, Wrapper>;

	protected:

		virtual void AddAppendInfoRef() const = 0;
		virtual void SubAppendInfoRef() const = 0;
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

		virtual void TriggerExecute(ContextWrapper& wrapper, AppendInfo* trigger_append_info) = 0;
		virtual void TriggerTerminal(ContextWrapper& wrapper, AppendInfo* trigger_append_info) noexcept = 0;

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

	struct Property
	{
		Priority priority = Priority::Normal;
		std::variant<std::monostate, std::size_t, std::thread::id> category;
		std::u8string_view node_name;
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
		virtual Property GetTaskNodeProperty() const = 0;
		virtual AppendInfo* GetAppendInfo() const = 0;
		virtual bool Commit(Node& target, Property property, AppendInfo::Ptr append_info = {}, Trigger::Ptr trigger = {}, AppendInfo::Ptr trigger_info = {}) = 0;
		virtual bool CommitDelay(Node& target, std::chrono::steady_clock::time_point target_time_point, Property property, AppendInfo::Ptr append_info = {}, Trigger::Ptr trigger = {}, AppendInfo::Ptr trigger_info = {}) = 0;
		virtual Node& GetCurrentTaskNode() const = 0;
	};
	

	export struct Node
	{

		struct Wrapper
		{
			void AddRef(Node const* ptr) { ptr->AddTaskNodeRef(); }
			void SubRef(Node const* ptr) { ptr->SubTaskNodeRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<Node, Wrapper>;

		template<typename FunT>
		static Ptr CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, ContextWrapper&>);

		virtual void TaskExecute(ContextWrapper& status) = 0;
		virtual void TaskTerminal(ContextWrapper& property) noexcept {};
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
			AppendInfo::Ptr append_info;
			Trigger::Ptr trigger;
			AppendInfo::Ptr trigger_info;
		};

		struct TimedNodeTuple
		{
			NodeTuple node_tuple;
			std::chrono::steady_clock::time_point request_time;
		};

		std::optional<NodeTuple> PopNode(std::chrono::steady_clock::time_point current_time);
		std::optional<NodeTuple> ForcePopNode();

		bool InsertNode(NodeTuple node);
		bool InsertDelayNode(NodeTuple node, std::chrono::steady_clock::time_point target_time_point);
		std::size_t GetTaskCount() const { return task_count; }
		void MarkNodeFinish() { task_count -= 1; }

	protected:

		NodeSequencer(Potato::IR::MemoryResourceRecord record, std::pmr::memory_resource* resource);

		std::atomic_size_t task_count = 0;

		std::mutex node_deque_mutex;
		std::optional<std::chrono::steady_clock::time_point> min_time_point;
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

		bool ExecuteContextThreadOnce(ThreadExecuteContext& execute_context, std::chrono::steady_clock::time_point now, bool accept_global = true, std::size_t group_id = std::numeric_limits<std::size_t>::max());

		bool CheckNodeSequencerEmpty();

		void ExecuteContextThreadUntilNoExsitTask(std::size_t group_id = std::numeric_limits<std::size_t>::max());
		bool Commit(Node& target, Property property, AppendInfo::Ptr append_info = {}, Trigger::Ptr trigger = {}, AppendInfo::Ptr trigger_info = {});
		bool CommitDelay(Node& target, std::chrono::steady_clock::time_point target_time_point, Property property, AppendInfo::Ptr append_info = {}, Trigger::Ptr trigger = {}, AppendInfo::Ptr trigger_info = {});

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
		void FlushNodeSequencer(Status state, NodeSequencer& target_sequence, std::size_t group_id);
		bool ExecuteNodeSequencer(NodeSequencer& target_sequence, std::size_t group_id, std::chrono::steady_clock::time_point now_time);
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

	template<typename FunT>
	auto Node::CreateLambdaNode(FunT&& func, std::pmr::memory_resource* resource)
		->Ptr requires(std::is_invocable_v<FunT, ContextWrapper&>)
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