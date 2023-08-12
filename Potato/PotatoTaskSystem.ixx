module;

#include <cassert>

export module PotatoTaskSystem;

export import PotatoSTD;
export import PotatoMisc;
export import PotatoPointer;


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
		void ExecuteAndRemoveAllTask();

		bool Commit(Task::Ptr TaskPtr, bool RequireUnique = false);

	private:

		TaskContext(std::pmr::memory_resource* Resource);

		void AddStrongRef() { SRef.AddRef(); }
		void SubStrongRef() ;
		void AddWeakRef() { WRef.AddRef(); }
		void SubWeakRef();
		bool TryAddStrongRef() { return SRef.TryAddRefNotFromZero(); }
		std::tuple<bool, Task::Ptr> TryGetTopTask();

		std::pmr::memory_resource* Resource;

		std::mutex ThreadMutex;
		std::vector<std::jthread, std::pmr::polymorphic_allocator<std::jthread>> Threads;

		struct WaittingTaskT
		{
			std::size_t Priority;
			Task::Ptr Task;
		};

		std::mutex TaskMutex;
		std::deque<WaittingTaskT,std::pmr::polymorphic_allocator<WaittingTaskT>> WaitingTask;
		std::size_t StartTask = 0;

		friend struct Pointer::SWSubWrapperT;
		friend struct Pointer::SWSubWrapperT;

		Misc::AtomicRefCount SRef;
		Misc::AtomicRefCount WRef;
	};

	/*
	template<typename DriverT>
	struct TaskReference : public BaseTaskReference
	{
		void SubStrongRef() {
			if (RefCount.SubRef())
			{
				static_cast<DriverT*>(this)->StrongRelease();
			}
		}
		
		void SubWeakRef()
		{
			if (WRefCount.SubRef())
			{
				static_cast<DriverT*>(this)->WeakRelease();
			}
		}

	};

	struct TaskContext : protected TaskReference<TaskContext>
	{

		using Ptr = Potato::SP::StrongPtr<TaskContext>;
		using WPtr = Potato::SP::WeakPtr<TaskContext>;

		static Ptr Create(std::size_t ThreadCount =
			std::thread::hardware_concurrency() - 1,
			std::pmr::memory_resource* MemoryPool = std::pmr::new_delete_resource(), std::pmr::memory_resource* SelfAllocator = std::pmr::new_delete_resource());

		enum class StatusE
		{
			Normal
		};

		enum class PriorityE : std::size_t
		{
			High = 100,
			Normal = 1000,
			Low = 10000,
		};

		constexpr std::size_t operator*(PriorityE Priority) {
			return static_cast<std::size_t>(Priority);
		}

		struct Info
		{
			std::u16string Name;
			std::size_t Priority = 1000;
		};

		struct Task
		{

			using Ptr = Potato::SP::IntrusivePtr<Task>;

			virtual const Info GetInfo() const = 0;

		protected:

			virtual void AddRef() = 0;
			virtual void SubRef() = 0;

			virtual void Execute(StatusE Status, TaskContext&) = 0;

			friend struct Potato::SP::IntrusiveSubWrapperT;
			friend struct TaskContext;
			friend struct TaskReference<Task>;
		};

		template<typename TaskT, typename ...OTher>
		Task::Ptr CreateTask(Info Info, OTher&& ...OT);

		template<typename Func>
		Task::Ptr CreateLambdaTask(Info Info, Func&& Fun) {
			return CreateTask<std::remove_reference_t<Func>>(std::move(Info), std::forward<Func>(Fun));
		}

		void Commit(Task::Ptr RequireTask);

	private:

		void StopTaskImp();

		void StrongRelease();
		void WeakRelease();

		TaskContext(std::pmr::memory_resource* MemoryPool, std::pmr::memory_resource* SelfAllocator)
			: SelfAllocator(SelfAllocator), MemoryPool(MemoryPool), Threads(&this->MemoryPool), WaittingTask(&this->MemoryPool)
		{}

		~TaskContext() {}

		static void Executer(std::stop_token ST, WPtr P);
		
		std::pmr::memory_resource* SelfAllocator;
		std::pmr::synchronized_pool_resource MemoryPool;

		std::mutex ThreadMutex;
		std::vector<std::jthread, std::pmr::polymorphic_allocator<std::jthread>> Threads;

		struct WaittingTaskT
		{
			std::size_t Priority;
			Task::Ptr Task;
			bool operator<(WaittingTaskT const& T2) const { return Priority < T2.Priority; }
		};

		std::mutex TaskMutex;
		std::size_t BasePriority = 0;
		std::priority_queue<WaittingTaskT, std::vector<WaittingTaskT, std::pmr::polymorphic_allocator<WaittingTaskT>>> WaittingTask;

		friend struct Potato::SP::SWSubWrapperT;
		friend struct Potato::SP::SWSubWrapperT;
		friend struct TaskReference<TaskContext>;
		template<typename TaskImpT>
		friend struct TaskImp;
	};
	*/
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

module : private;

namespace Potato::Task
{
	/*
	auto TaskContext::Create(std::size_t ThreadCount, std::pmr::memory_resource* MemoryPool, std::pmr::memory_resource* SelfAllocator)
		-> Ptr
	{
		ThreadCount = std::max(ThreadCount, std::size_t{1});
		assert(MemoryPool != nullptr && SelfAllocator != nullptr);
		auto ConAdress = SelfAllocator->allocate(sizeof(TaskContext), alignof(TaskContext));
		if (ConAdress != nullptr)
		{
			auto ConPtr = new (ConAdress) TaskContext{ MemoryPool, SelfAllocator };
			Ptr P{ ConPtr };
			WPtr WP = P;
			for (std::size_t I = 0; I < ThreadCount; ++I)
			{
				P->Threads.emplace_back([WP](std::stop_token ST) {
					TaskContext::Executer(std::move(ST), std::move(WP));
				});
			}
			return P;
		}
		return {};
	}

	void TaskContext::StopTaskImp()
	{
		auto Curid = std::this_thread::get_id();
		std::lock_guard lg(ThreadMutex);
		for (auto& Ite : Threads)
		{
			Ite.request_stop();
			if (Curid == Ite.get_id())
			{
				Ite.detach();
			}
		}
		Threads.clear();
	}

	void TaskContext::Executer(std::stop_token ST, WPtr P)
	{
		while (!ST.stop_requested())
		{
			Ptr Upgrade = P;
			if (Upgrade)
			{
				Task::Ptr TaskPtr;
				bool NeedSleep = false;
				if (Upgrade->ThreadMutex.try_lock())
				{
					std::lock_guard lg(Upgrade->ThreadMutex, std::adopt_lock_t{});
					if (Upgrade->WaittingTask.empty())
					{
						NeedSleep = true;
					}
					else {
						auto TopTask = Upgrade->WaittingTask.top();
						TaskPtr = std::move(TopTask.Task);
						Upgrade->WaittingTask.pop();
					}
				}else
					std::this_thread::yield();
				
				if (TaskPtr)
				{
					TaskPtr->Execute(StatusE::Normal, *Upgrade);
					Upgrade.Reset();
					std::this_thread::yield();
				}
				else {
					Upgrade.Reset();
					std::this_thread::sleep_for(std::chrono::microseconds{1});
				}
			}
		}
	}

	void TaskContext::StrongRelease()
	{
		StopTaskImp();
	}

	void TaskContext::WeakRelease()
	{
		auto OldSelfAllocator = SelfAllocator;
		StopTaskImp();
		this->~TaskContext();
		OldSelfAllocator->deallocate(this, sizeof(TaskContext), alignof(TaskContext));
	}

	void TaskContext::Commit(Task::Ptr RequireTask)
	{
		if (RequireTask)
		{
			std::lock_guard lg(TaskMutex);
			WaittingTask.emplace(
				BasePriority + TargetPriority,
				TaskP
			);
		}
	}
	*/
}

