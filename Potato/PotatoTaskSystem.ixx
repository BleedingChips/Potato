module;

#include <cassert>

export module PotatoTaskSystem;

import std;
import PotatoMisc;
import PotatoPointer;


export namespace Potato::Task
{

	enum class ExecuteStatus : std::size_t
	{
		Normal,
		Close,
	};

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

	struct TaskInit
	{
		std::wstring_view Name;
		std::size_t Priority = *TaskPriority::Normal;
		std::pmr::memory_resource* Resource = std::pmr::new_delete_resource();
	};

	struct Task
	{
		using Ptr = Pointer::IntrusivePtr<Task>;

		virtual std::wstring_view GetTaskName() const = 0;
		virtual std::size_t GetTaskPriority() const = 0;

		template<typename TaskT, typename ...AT>
			requires (std::is_constructible_v<TaskT, AT&&...>)
		static Ptr Create(TaskInit Init, AT&& ...at);

		template<typename FuncT>
		static Ptr CreateLambda(TaskInit Init, FuncT&& Fun)
		{
			return Create<std::remove_cvref_t<FuncT>>(std::move(Init), std::forward<FuncT>(Fun));
		}

	private:
		
		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;
		virtual void Execute(ExecuteStatus Status, TaskContext& Context) = 0;

		friend struct TaskContext;
		friend struct Pointer::IntrusiveSubWrapperT;
	};

	struct ExecuteInfo
	{
		ExecuteStatus Status;
		TaskContext& Context;
		Task* Self;
		operator bool() const { return Status == ExecuteStatus::Normal; }
	};

	struct TaskContext
	{
		using Ptr = Pointer::StrongPtr<TaskContext>;
		using WPtr = Pointer::WeakPtr<TaskContext>;

		static Ptr Create(std::pmr::memory_resource* Resource = std::pmr::new_delete_resource());
		bool FireThreads(std::size_t TaskCount = std::thread::hardware_concurrency() - 1);
		std::size_t CloseThreads();
		void ExecuteAndRemoveAllTask(std::pmr::memory_resource* TempResource = std::pmr::new_delete_resource());

		bool CommitTask(Task::Ptr TaskPtr);
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::time_point TimePoint);

	private:

		TaskContext(std::pmr::memory_resource* Resource);

		void AddStrongRef() { SRef.AddRef(); }
		void SubStrongRef() ;
		void AddWeakRef() { WRef.AddRef(); }
		void SubWeakRef();
		bool TryAddStrongRef() { return SRef.TryAddRefNotFromZero(); }

		bool TryPushDelayTask();
		std::tuple<bool, Task::Ptr> TryGetTopTask();

		std::pmr::memory_resource* Resource;

		std::mutex ThreadMutex;
		std::vector<std::jthread, std::pmr::polymorphic_allocator<std::jthread>> Threads;

		struct ReadyTaskT
		{
			std::size_t Priority;
			Task::Ptr Task;
		};

		struct DelayTaskT
		{
			std::chrono::system_clock::time_point DelayTimePoint;
			Task::Ptr Task;
		};

		std::jthread TimmerThread;
		std::mutex TaskMutex;
		bool EnableInsertTask = true;
		std::vector<DelayTaskT, std::pmr::polymorphic_allocator<DelayTaskT>> DelayTasks;
		std::size_t DelayTaskIte = 0;
		std::vector<ReadyTaskT,std::pmr::polymorphic_allocator<ReadyTaskT>> ReadyTasks;
		std::size_t ReadyTasksIte = 0;

		friend struct Pointer::SWSubWrapperT;
		friend struct Pointer::SWSubWrapperT;

		Misc::AtomicRefCount SRef;
		Misc::AtomicRefCount WRef;
	};
}

namespace Potato::Task
{

	template<typename TaskImpT>
	struct TaskImp : public Task
	{

		virtual void AddRef() const override
		{
			RefCount.AddRef();
		}

		virtual void SubRef() const override
		{
			if (RefCount.SubRef())
			{
				auto OldResource = Resource;
				this->~TaskImp();
				OldResource->deallocate( const_cast<TaskImp*>(this), sizeof(TaskImp), alignof(TaskImp));
			}
		}

		virtual std::wstring_view GetTaskName() const override{ return std::wstring_view{ TaskName }; }
		virtual std::size_t GetTaskPriority() const override { return TasklPriority; }

		template<typename ...AT>
		TaskImp(TaskInit Init, AT&& ...at)
			: Resource(Init.Resource), TaskName(Init.Name, Init.Resource), TasklPriority(Init.Priority), TaskInstance(std::forward<AT>(at)...) {}

		virtual void Execute(ExecuteStatus Status, TaskContext& Context) override
		{
			ExecuteInfo Info{
				Status,
				Context,
				this
			};
			TaskInstance.operator()(Info);
		}

	private:

		Potato::Misc::AtomicRefCount RefCount;
		std::pmr::memory_resource* Resource;
		std::basic_string<wchar_t, std::char_traits<wchar_t>, std::pmr::polymorphic_allocator<wchar_t>> TaskName;
		std::size_t TasklPriority;
		TaskImpT TaskInstance;
	};

	template<typename TaskT, typename ...AT>
		requires (std::is_constructible_v<TaskT, AT&&...>)
	auto Task::Create(TaskInit Init, AT&& ...OT)  ->Ptr
	{
		assert(Init.Resource != nullptr);
		auto Adress = Init.Resource->allocate(sizeof(TaskImp<TaskT>), alignof(TaskImp<TaskT>));
		if (Adress != nullptr)
		{
			auto TaskPtr = new (Adress) TaskImp<TaskT> {Init, std::forward<AT>(OT)...};
			Task::Ptr TaskP{ TaskPtr };
			
			return TaskP;
		}
		return {};
	}
}