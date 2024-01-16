module;

#include <cassert>

export module PotatoTaskSystem;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;


export namespace Potato::Task
{

	export struct TaskContext;

	enum class Priority : std::size_t
	{
		VeryHigh = 64,
		High = 32,
		Normal = 16,
		Low = 4,
		VeryLow = 1,
	};

	struct TaskProperty
	{

		enum class Category
		{
			GLOBAL_TASK,
			GROUP_TASK,
			THREAD_TASK
		};

		Priority priority = Priority::Normal;
		std::u8string_view name = u8"Unnamed Task";
		std::array<std::size_t, 2> user_data;
		Category category = Category::GLOBAL_TASK;
		std::size_t group_id = 0;
		std::thread::id thread_id;
	};

	enum class Status : std::size_t
	{
		Normal,
		Close
	};

	struct ThreadProperty
	{
		std::size_t group_id = 0;
		bool execute_global_task = true;
		bool execute_group_task = true;
		std::u8string_view name;
	};

	struct ExecuteStatus
	{
		Status content_status = Status::Normal;
		TaskContext& context;
		TaskProperty task_property;
		
		Status thread_status = Status::Normal;
		std::thread::id thread_id;
		ThreadProperty thread_property;
	};

	struct Task
	{
		using Ptr = Pointer::IntrusivePtr<Task>;

		template<typename FunT>
		static Ptr CreateLambdaTask(FunT&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, ExecuteStatus& , Task::Ptr>)
		;

	protected:
		
		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;

		virtual void operator()(ExecuteStatus& status) = 0;
		virtual ~Task() = default;

		friend struct TaskContext;
		friend struct Potato::Pointer::DefaultIntrusiveWrapper;
	};


	/*
	struct TaskQueue
	{

		struct TaskTuple
		{
			TaskProperty property;
			Task::Ptr task;
		};

		void InsertTask(Task::Ptr task, TaskProperty property);
		void InsertTask(Task::Ptr task, TaskProperty property, std::chrono::system_clock::time_point time_point);
		std::optional<TaskTuple> PopTask(ThreadProperty property, std::thread::id thread_id, std::chrono::system_clock::time_point current_time, bool accept_all_group);
		std::size_t CheckTimeTask(ThreadProperty property, std::thread::id thread_id, bool accept_all_group);
		TaskQueue(std::pmr::memory_resource* resource);

	protected:

		static bool Accept(TaskProperty const& property, ThreadProperty const& thread_property, std::thread::id thread_id, bool accept_all_group);

		struct TimeTuple
		{
			TaskTuple tuple;
			std::chrono::system_clock::time_point time_point;
		};

		struct LineUpTuple
		{
			TaskTuple tuple;
			std::size_t priority;
		};

		std::chrono::system_clock::time_point min_time_point = std::chrono::system_clock::time_point::max();

		std::pmr::vector<TimeTuple> time_task;
		std::pmr::vector<LineUpTuple> line_up_task;
		bool already_add_priority = false;

	};
	*/

	export struct TaskContext : public Pointer::DefaultControllerViewerInterface
	{
		using Ptr = Pointer::ControllerPtr<TaskContext>;

		static Ptr Create(std::pmr::memory_resource* resource = std::pmr::get_default_resource());

		static std::size_t GetSuggestThreadCount();

		bool AddGroupThread(ThreadProperty property, std::size_t thread_count = 1);
		std::size_t GetRandomThreadID(std::size_t group_id);
		std::size_t GetRandomThreadID();

		void RequestCloseThread(std::size_t thread_id);
		void RequestCloseGroupThread(std::size_t thread_id);
		void RequestCloseAllThread();

		void ProcessTask(std::optional<std::size_t> override_group_id = {}, bool flush_timer = false);

		std::size_t CloseThread();
		bool CommitTask(Task::Ptr task, TaskProperty property = {});
		bool CommitDelayTask(Task::Ptr task, std::chrono::steady_clock::time_point time_point, TaskProperty property = {});
		bool CommitDelayTask(Task::Ptr task, std::chrono::steady_clock::duration duration, TaskProperty property = {})
		{
			return CommitDelayTask(std::move(task), std::chrono::steady_clock::now() + duration, property);
		}

		virtual ~TaskContext();

	protected:

		TaskContext(Potato::IR::MemoryResourceRecord record);

	private:

		struct TaskTuple
		{
			TaskProperty property;
			Task::Ptr task;
		};

		static bool Accept(TaskProperty const& property, ThreadProperty const& thread_property, std::thread::id thread_id, bool accept_all_group);
		std::optional<TaskTuple> PopLineUpTask(ThreadProperty property, std::thread::id thread_id, std::chrono::system_clock::time_point current_time, bool accept_all_group);

		virtual void ViewerRelease() override;
		virtual void ControllerRelease() override;

		void TimerThreadExecute(std::stop_token ST);
		void LineUpThreadExecute(std::stop_token ST, std::size_t index, ThreadProperty property, std::thread::id thread_id);

		using WPtr = Pointer::ViewerPtr<TaskContext>;

		Potato::IR::MemoryResourceRecord record;

		

		std::shared_mutex thread_mutex;
		
		struct ThreadCore
		{
			ThreadProperty property;
			std::thread::id thread_id;
			std::jthread thread;
		};
		std::pmr::vector<ThreadCore> thread;

		std::mutex timer_mutex;
		std::jthread timer_thread;
		std::condition_variable timer_cv;
		std::chrono::steady_clock::time_point last_time_point = std::chrono::steady_clock::time_point::max();
		struct TimerTaskTuple
		{
			TaskTuple tuple;
			std::chrono::steady_clock::time_point time_point;
		};
		std::pmr::vector<TimerTaskTuple> timed_task;

		std::shared_mutex line_up_thread_mutex;
		struct LineUpTuple
		{
			TaskTuple tuple;
			std::size_t priority;
		};
		std::pmr::vector<LineUpTuple> line_up_task;
		std::size_t total_task_count = 0;
		Status status = Status::Normal;
		bool already_add_priority = false;
	};

}

namespace Potato::Task
{

	template<typename TaskImpT>
	struct TaskImp : public Task, public Potato::Pointer::DefaultIntrusiveInterface
	{

		virtual void Release() override
		{
			auto Rec = Record;
			this->~TaskImp();
			Rec.Deallocate();
		}

		template<typename FunT>
		TaskImp(Potato::IR::MemoryResourceRecord Record, FunT&& Fun)
			: Record(Record), TaskInstance(std::forward<FunT>(Fun))
		{
			
		}

		virtual void operator()(ExecuteStatus& Status) override
		{
			Task::Ptr ThisPtr{this};
			TaskInstance.operator()(Status, std::move(ThisPtr));
		}

	protected:

		virtual void AddRef() const override { DefaultIntrusiveInterface::AddRef(); }
		virtual void SubRef() const override { DefaultIntrusiveInterface::SubRef(); }

	private:

		Potato::IR::MemoryResourceRecord Record;
		TaskImpT TaskInstance;
	};

	template<typename FunT>
	Task::Ptr Task::CreateLambdaTask(FunT&& Func, std::pmr::memory_resource* Resource)
		requires(std::is_invocable_v<FunT, ExecuteStatus&, Task::Ptr>)
	{
		using Type = TaskImp<std::remove_cvref_t<FunT>>;
		assert(Resource != nullptr);
		auto Record = Potato::IR::MemoryResourceRecord::Allocate<Type>(Resource);
		if(Record)
		{
			return new (Record.Get()) Type{ Record, std::forward<FunT>(Func)};
		}
		return {};
	}
}