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
		std::wstring Name;
		std::size_t Priority = *TaskPriority::Normal;
		std::pmr::memory_resource* Resource = nullptr;
	};

	struct Task
	{
		using Ptr = Pointer::IntrusivePtr<Task>;

		virtual std::u16string_view GetTaskName() const = 0;
		virtual std::size_t GetTaskPriority() = 0;

		template<typename TaskT, typename ...AT>
		static Ptr Create(TaskInit Init, AT&& ...at);

	private:
		
		virtual void AddRef() const = 0;
		virtual void SubRef() const = 0;
		virtual void Execute(ExecuteStatus Status, TaskContext& Context);

		friend struct TaskContext;
		friend struct Pointer::IntrusiveSubWrapperT;
	};

	struct TaskContext
	{
		using Ptr = Pointer::IntrusivePtr<TaskContext>;

	private:

		

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
	/*
	template<typename TaskImpT>
	struct TaskImp : public TaskContext::Task
	{

		void AddRef() override
		{
			RefCount.AddRef();
		}

		void SubRef() override
		{
			if (RefCount.SubRef())
			{
				auto OldResource = Resource;
				this->~TaskImp();
				OldResource->deallocate(this, sizeof(TaskImp), alignof(TaskImp));
			}
		}

		template<typename ...AT>
		TaskImp(TaskContext::Info Info, std::pmr::memory_resource* Resource, AT&& ...at)
			: TaskInfo(std::move(Info)), Resource(Resource),
			TaskInstance(std::forward<AT>(at)...) {}

		virtual void Execute(TaskContext::StatusE Status, TaskContext& Context) override {
			TaskInstance.operator()(Status, Context);
		}

		virtual TaskContext::Info const& GetInfo() const override
		{
			return TaskInfo;
		}

	private:

		TaskContext::Info TaskInfo;
		Potato::Misc::AtomicRefCount RefCount;
		std::pmr::memory_resource* Resource;
		TaskImpT TaskInstance;
	};

	template<typename TaskT, typename ...OTher>
	auto TaskContext::CreateTask(TaskContext::Info Info, OTher&& ...OT) ->Task::Ptr
	{
		auto TargetPriority = Info.Priority;
		auto Adress = MemoryPool.allocate(sizeof(TaskImp<TaskT>), alignof(TaskImp<TaskT>));
		if (Adress != nullptr)
		{
			auto TaskPtr = new (Adress) TaskImp<TaskT> {std::move(Info), & MemoryPool, std::forward<OTher>(OT)...};
			Task::Ptr TaskP{ TaskPtr };
			
			return TaskP;
		}
		return {};
	}
	*/
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

