module;

#include <cassert>

export module PotatoTaskSystem;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;


export namespace Potato::Task
{

	enum class TaskPriority : std::size_t
	{
		High = 100,
		Normal = 1000,
		Low = 10000,
	};

	constexpr std::size_t operator*(TaskPriority Priority) {
		return static_cast<std::size_t>(Priority);
	}

	export struct TaskContext;

	struct TaskProperty
	{
		std::size_t TaskPriority = *TaskPriority::Normal;
		
		std::u8string_view TaskName = u8"Unnamed Task";
		std::size_t AppendData = 0;
		std::size_t AppendData2 = 0;
		bool FrontEndTask = true;
	};

	enum class TaskContextStatus : std::size_t
	{
		Normal,
		Close,
	};

	struct ExecuteStatus
	{
		TaskContextStatus Status = TaskContextStatus::Normal;
		TaskProperty Property;
		TaskContext& Context;

		operator bool() const { return Status == TaskContextStatus::Normal; }
	};

	struct Task
	{
		using Ptr = Pointer::IntrusivePtr<Task>;

		template<typename FunT>
		static Ptr CreatLambdaTask(FunT&& Func, std::pmr::memory_resource* Resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, ExecuteStatus& , Task::Ptr>)
		;

	protected:
		
		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;

		virtual void operator()(ExecuteStatus& Status) = 0;
		virtual ~Task() = default;

		friend struct TaskContext;
		friend struct Potato::Pointer::DefaultIntrusiveWrapper;
	};

	struct TaskQueue
	{

		struct Tuple
		{
			TaskProperty Property;
			Task::Ptr Task;
		};

		Tuple PopFrontEndTask();
		Tuple PopBackEndTask();

		bool Insert(Task::Ptr Task, TaskProperty Property);

		TaskQueue(std::pmr::memory_resource* resource)
			: AllTask(resource)
		{
			
		}

		std::size_t Count() const;

	protected:

		Tuple TopFrontEnd;
		Tuple TopBackEnd;
		std::pmr::vector<Tuple> AllTask;
	};

	struct TimedTaskQueue
	{

		TimedTaskQueue(std::pmr::memory_resource* resource)
			: TimedTasks(resource)
		{
			
		}

		bool Insert(Task::Ptr Task, TaskProperty Property, std::chrono::system_clock::time_point TimePoint);
		std::size_t PopTask(TaskQueue& TaskQueue, std::chrono::system_clock::time_point CurrentTime);

		std::size_t Count() const { return TimedTasks.size(); }

	protected:

		struct DelayTaskT
		{
			std::chrono::system_clock::time_point DelayTimePoint;
			TaskProperty Property;
			Task::Ptr Task;
		};

		std::chrono::system_clock::time_point ClosestTimePoint = std::chrono::system_clock::time_point::max();
		std::pmr::vector<DelayTaskT> TimedTasks;

	};

	export struct TaskContext : public Pointer::DefaultControllerViewerInterface
	{
		using Ptr = Pointer::ControllerPtr<TaskContext>;

		static Ptr Create(std::pmr::memory_resource* Resource = std::pmr::get_default_resource());
		bool FireThreads(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1)
		{
			return FireThreads(ThreadCount, ThreadCount);
		}
		bool FireThreads(std::size_t ThreadCount, std::size_t FrontEndThreadCount);

		std::size_t CloseThreads();
		void FlushTask(std::chrono::system_clock::time_point RequireTP = std::chrono::system_clock::time_point::max());
		void WaitTask();

		bool CommitTask(Task::Ptr TaskPtr, TaskProperty Property = {});
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::time_point TimePoint, TaskProperty Property = {});
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::duration Duration, TaskProperty Property = {})
		{
			return CommitDelayTask(std::move(TaskPtr), std::chrono::system_clock::now() + Duration, Property);
		}

		virtual ~TaskContext();

	protected:

		TaskContext(Potato::IR::MemoryResourceRecord record);

	private:

		virtual void ViewerRelease() override;
		virtual void ControllerRelease() override;

		using WPtr = Pointer::ViewerPtr<TaskContext>;

		struct ExecuteContext
		{
			std::size_t LastingTask = 1;
			bool LastExecute = false;
			bool Locked = false;
			TaskContextStatus Status = TaskContextStatus::Normal;
			bool IsFrontEndThread = false;
		};

		void ThreadExecute(std::stop_token ST);

		void ExecuteOnce(ExecuteContext& Context, std::chrono::system_clock::time_point CurrentTime);

		Potato::IR::MemoryResourceRecord Record;

		std::mutex ThreadMutex;
		std::pmr::vector<std::jthread> Threads;

		std::mutex TaskMutex;
		std::size_t FrontEndThreadCount = 1;
		std::size_t CurrentFrontEndThreadCount = 0;
		TaskContextStatus Status = TaskContextStatus::Normal;
		std::size_t LastingTask = 0;
		

		TimedTaskQueue DelayTasks;
		TaskQueue ReadyTasks;
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
	Task::Ptr Task::CreatLambdaTask(FunT&& Func, std::pmr::memory_resource* Resource)
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