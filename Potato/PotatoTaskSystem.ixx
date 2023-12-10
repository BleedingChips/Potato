module;

#include <cassert>

export module PotatoTaskSystem;

import std;
import PotatoMisc;
import PotatoPointer;


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

	struct TaskContext;

	struct TaskProperty
	{
		std::size_t TaskPriority = *TaskPriority::Normal;
		std::u8string_view TaskName = u8"Unnamed Task";
		std::size_t AppendData = 0;
		std::size_t AppendData2 = 0;
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

	struct TaskContext : public Pointer::DefaultControllerViewerInterface
	{
		using Ptr = Pointer::ControllerPtr<TaskContext>;

		static Ptr Create(std::pmr::memory_resource* Resource = std::pmr::get_default_resource());
		bool FireThreads(std::size_t TaskCount = std::thread::hardware_concurrency() - 1);

		std::size_t CloseThreads();
		void FlushTask();
		void WaitTask();

		bool CommitTask(Task::Ptr TaskPtr, TaskProperty Property = {});
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::time_point TimePoint, TaskProperty Property = {});
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::duration Duration, TaskProperty Property = {})
		{
			return CommitDelayTask(std::move(TaskPtr), std::chrono::system_clock::now() + Duration, Property);
		}

		~TaskContext();

	private:

		virtual void ViewerRelease() override;
		virtual void ControllerRelease() override;

		using WPtr = Pointer::ViewerPtr<TaskContext>;

		TaskContext(std::pmr::memory_resource* Resource);

		struct ExecuteContext
		{
			std::size_t LastingTask = 1;
			bool LastExecute = false;
			bool Locked = false;
			TaskContextStatus Status = TaskContextStatus::Normal;
		};

		void ExecuteOnce(ExecuteContext& Context, std::chrono::system_clock::time_point CurrentTime);

		std::pmr::memory_resource* Resource;

		std::mutex ThreadMutex;
		std::pmr::vector<std::jthread> Threads;

		struct ReadyTaskT
		{
			TaskProperty Property;
			Task::Ptr Task;
		};

		struct DelayTaskT
		{
			std::chrono::system_clock::time_point DelayTimePoint;
			TaskProperty Property;
			Task::Ptr Task;
		};

		std::mutex TaskMutex;
		TaskContextStatus Status = TaskContextStatus::Normal;
		std::size_t LastingTask = 0;
		std::chrono::system_clock::time_point ClosestTimePoint;
		std::pmr::vector<DelayTaskT> DelayTasks;
		std::pmr::vector<ReadyTaskT> ReadyTasks;
	};

}

namespace Potato::Task
{

	template<typename TaskImpT>
	struct TaskImp : public Task, public Potato::Pointer::DefaultIntrusiveInterface
	{

		virtual void Release() override
		{
			auto OldResource = Resource;
			this->~TaskImp();
			OldResource->deallocate(const_cast<TaskImp*>(this), sizeof(TaskImp), alignof(TaskImp));
		}

		template<typename FunT>
		TaskImp(std::pmr::memory_resource* Resource, FunT&& Fun)
			: Resource(Resource), TaskInstance(std::forward<FunT>(Fun))
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

		std::pmr::memory_resource* Resource;
		TaskImpT TaskInstance;
	};

	template<typename FunT>
	Task::Ptr Task::CreatLambdaTask(FunT&& Func, std::pmr::memory_resource* Resource)
		requires(std::is_invocable_v<FunT, ExecuteStatus&, Task::Ptr>)
	{
		using Type = TaskImp<std::remove_cvref_t<FunT>>;
		assert(Resource != nullptr);
		auto Adress = Resource->allocate(sizeof(Type), alignof(Type));
		if(Adress != nullptr)
		{
			return new (Adress) Type{ Resource, std::forward<FunT>(Func)};
		}
		return {};
	}
}