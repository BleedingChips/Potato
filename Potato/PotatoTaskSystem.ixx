module;

#include <cassert>

export module PotatoTaskSystem;

export import PotatoSTD;
export import PotatoMisc;
export import PotatoSmartPtr;


export namespace Potato::Task
{

	struct BaseTaskReference
	{
		void AddStrongRef() { RefCount.AddRef(); }
		void AddWeakRef() { WRefCount.AddRef(); }
		bool TryAddStrongRef() { return RefCount.TryAddRefNotFromZero(); }

	protected:

		mutable Misc::AtomicRefCount RefCount;
		mutable Misc::AtomicRefCount WRefCount;
	};

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


		struct Task : protected TaskReference<Task>
		{

			enum class PriorityT : std::size_t
			{
				High = 100,
				Normal = 1000,
				Low = 10000,
			};

			struct Info
			{
				std::u16string Name;
				std::size_t Priority = 1000;
			};

			using Ptr = Potato::SP::StrongPtr<Task>;
			using WPtr = Potato::SP::WeakPtr<Task>;

			Task(TaskContext::Ptr Owner, Info SelfInfo)
				: Owner(std::move(Owner)) {}

			TaskContext::Ptr GetOwner() const { return Owner; }

			void Commit();

		protected:

			virtual void StrongRelease() = 0;
			virtual void WeakRelease() = 0;

			virtual void Execute() = 0;
			
			TaskContext::Ptr Owner;

			friend struct Potato::SP::SWSubWrapperT;
			friend struct Potato::SP::SWSubWrapperT;
			friend struct TaskContext;
			friend struct TaskReference<Task>;
		};

		template<typename TaskT, typename ...OTher>
		Task::Ptr CreateTask(Task::Info Info, OTher&& ...OT);

		template<typename Func>
		Task::Ptr CreateLambdaTask(Task::Info Info, Func&& Fun) {
			return CreateTask<std::remove_reference_t<Func>>(std::move(Info), std::forward<Func>(Fun));
		}

	private:

		void Commit(Task::Ptr TaskPtr);

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
			Task::WPtr Task;
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

}

namespace Potato::Task
{
	template<typename TaskImpT>
	struct TaskImp : public TaskContext::Task
	{
		std::optional<TaskImpT> Task;

		template<typename ...AT>
		TaskImp(TaskContext::Ptr Owner, Task::Info Info, AT&& ...at)
			: TaskContext::Task(std::move(Owner), std::move(Info)), Task(std::in_place_t{}, std::forward<AT>(at)...) {}


		virtual void Execute() override {
			Task->operator()(GetOwner());
		}
		virtual void StrongRelease() override {
			Task.reset();
		}
		virtual void WeakRelease() override {
			auto OldOwner = std::move(Owner);
			this->~TaskImp();
			OldOwner->MemoryPool.deallocate(this, sizeof(TaskImp), alignof(TaskImp));
		}
	};

	template<typename TaskT, typename ...OTher>
	auto TaskContext::CreateTask(Task::Info Info, OTher&& ...OT) ->Task::Ptr
	{
		Ptr ThisPtr{this};
		auto Adress = MemoryPool.allocate(sizeof(TaskImp<TaskT>), alignof(TaskImp<TaskT>));
		auto TaskPtr = new (Adress) TaskImp<TaskT> {std::move(ThisPtr), std::move(Info), std::forward<OTher>(OT)...};
		return Task::Ptr{TaskPtr};
	}
}

constexpr std::size_t operator*(Potato::Task::TaskContext::Task::PriorityT Priority) { return static_cast<std::size_t>(Priority); }

module : private;

namespace Potato::Task
{

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
						TaskPtr = TopTask.Task;
						Upgrade->WaittingTask.pop();
					}
				}else
					std::this_thread::yield();
				Upgrade.Reset();
				if (TaskPtr)
				{
					TaskPtr->Execute();
					std::this_thread::yield();
				}
				else {
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

	void TaskContext::Commit(Task::Ptr TaskPtr)
	{
		std::lock_guard lg(TaskMutex);
		WaittingTaskT TaskInfo{
			100 + BasePriority,
			TaskPtr
		};
		WaittingTask.push(std::move(TaskInfo));
	}

	void TaskContext::Task::Commit()
	{
		Owner->Commit(Ptr{this});
	}
}

