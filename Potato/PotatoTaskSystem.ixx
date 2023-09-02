module;

#include <cassert>

export module PotatoTaskSystem;

import std;
import PotatoMisc;
import PotatoPointer;

export namespace Potato::Task
{
	struct ControlPtrDefaultWrapper
	{
		template<typename PtrT>
		void AddRef(PtrT* Ptr) { Ptr->AddRef(); Ptr->AddControlRef(); }
		template<typename PtrT>
		void SubRef(PtrT* Ptr) { Ptr->SubControlRef(); Ptr->SubRef(); }
	};

	struct ControlDefaultInterface : public Potato::Pointer::DefaultIntrusiveInterface
	{
		void AddControlRef() const { CRef.AddRef(); }
		void SubControlRef() const { if(CRef.SubRef()) const_cast<ControlDefaultInterface*>(this)->ControlRelease(); }
	protected:
		virtual void ControlRelease() = 0;
		mutable Misc::AtomicRefCount CRef;
		friend struct ControlPtrDefaultWrapper;
	};

}

export namespace Potato::Task
{

	template<typename PtrT>
	using ControlPtr = Potato::Pointer::SmartPtr<PtrT, Pointer::SmartPtrDefaultWrapper<ControlPtrDefaultWrapper>>;


	enum class ExecuteStatus : std::size_t
	{
		Normal,
		Close,
	};

	constexpr bool operator *(ExecuteStatus Status) { return Status == ExecuteStatus::Normal; }

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

	struct Task : public ControlDefaultInterface
	{
		using Ptr = ControlPtr<Task>;

		template<typename FunT>
		static Ptr CreatLambdaTask(FunT&& Func, std::u8string_view Name = u8"NoNameLambdaFunc", std::size_t Priority = *TaskPriority::Normal, std::pmr::memory_resource* Resource = std::pmr::get_default_resource());

		virtual std::u8string_view GetTaskName() const = 0;
		virtual std::size_t GetTaskPriority() const { return *TaskPriority::Normal; }

	protected:

		using WPtr = Pointer::IntrusivePtr<Task>;

		virtual void ControlRelease() override {};
		virtual void operator()(ExecuteStatus Status, TaskContext&) = 0;
		virtual ~Task() = default;

		friend struct TaskContext;
	};

	struct TaskContext : public ControlDefaultInterface
	{
		using Ptr = ControlPtr<TaskContext>;

		static Ptr Create(std::pmr::memory_resource* Resource = std::pmr::get_default_resource());
		bool FireThreads(std::size_t TaskCount = std::thread::hardware_concurrency() - 1);

		std::size_t CloseThreads();
		void FlushTask();
		void WaitTask();

		bool CommitTask(Task::Ptr TaskPtr);
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::time_point TimePoint);

		~TaskContext();

	private:

		void Release();
		void ControlRelease();

		using WPtr = Pointer::IntrusivePtr<TaskContext>;

		TaskContext(std::pmr::memory_resource* Resource);

		struct ExecuteContext
		{
			std::size_t LastingTask = 1;
			bool LastExecute = false;
			bool Locked = false;
			ExecuteStatus Status = ExecuteStatus::Normal;
		};

		void ExecuteOnce(ExecuteContext& Context, std::chrono::system_clock::time_point CurrentTime);

		std::pmr::memory_resource* Resource;

		std::mutex ThreadMutex;
		std::pmr::vector<std::jthread> Threads;

		struct ReadyTaskT
		{
			std::size_t Priority;
			Task::WPtr Task;
		};

		struct DelayTaskT
		{
			std::chrono::system_clock::time_point DelayTimePoint;
			Task::WPtr Task;
		};

		std::mutex TaskMutex;
		ExecuteStatus Status = ExecuteStatus::Normal;
		std::size_t LastingTask = 0;
		std::chrono::system_clock::time_point ClosestTimePoint;
		std::pmr::vector<DelayTaskT> DelayTasks;
		std::pmr::vector<ReadyTaskT> ReadyTasks;
	};
}

namespace Potato::Task
{

	template<typename TaskImpT>
	struct TaskImp : public Task
	{

		virtual void Release() override
		{
			auto OldResource = Resource;
			this->~TaskImp();
			OldResource->deallocate(const_cast<TaskImp*>(this), sizeof(TaskImp), alignof(TaskImp));
		}

		virtual std::u8string_view GetTaskName() const override{ return std::u8string_view{ TaskName }; }
		virtual std::size_t GetTaskPriority() const override { return TasklPriority; }

		template<typename FunT>
		TaskImp(std::u8string_view Name, std::size_t Priority, std::pmr::memory_resource* Resource, FunT&& Fun)
			: Resource(Resource), TaskName(Name), TasklPriority(Priority), TaskInstance(std::forward<FunT>(Fun))
		{
			
		}

		virtual void operator()(ExecuteStatus Status, TaskContext& Context) override
		{
			Task::Ptr ThisPtr{this};
			TaskInstance.operator()(Status, Context, std::move(ThisPtr));
		}

	private:

		std::pmr::memory_resource* Resource;
		std::u8string_view TaskName;
		std::size_t TasklPriority;
		TaskImpT TaskInstance;
	};

	template<typename FunT>
	Task::Ptr Task::CreatLambdaTask(FunT&& Func, std::u8string_view Name, std::size_t Priority, std::pmr::memory_resource* Resource)
	{
		using Type = TaskImp<std::remove_cvref_t<FunT>>;
		assert(Resource != nullptr);
		auto Adress = Resource->allocate(sizeof(Type), alignof(Type));
		if(Adress != nullptr)
		{
			return new (Adress) Type{ Name, Priority, Resource, std::forward<FunT>(Func)};
		}
		return {};
	}
}