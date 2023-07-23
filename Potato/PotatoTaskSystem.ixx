module;

#include <cassert>

export module PotatoTaskSystem;

export import PotatoSTD;
export import PotatoMisc;
export import PotatoSmartPtr;

namespace Potato::Task
{
	struct Task;
	struct TaskGraph;

	struct TaskSystem;

	/*
	struct TaskSystemReference
	{
		std::pmr::memory_resource* RefMemoryResource;
		std::pmr::synchronized_pool_resource MemoryPool;
		Misc::AtomicRefCount SRef;
		Misc::AtomicRefCount WRef;
		// todo
		//std::pmr::synchronized_pool_resource MemoryPool;

		TaskSystemReference(std::pmr::memory_resource* MemoryResouce)
			:  RefMemoryResource(MemoryResouce), MemoryPool(MemoryResouce)//, MemoryPool(MemoryResouce)
		{

		}

		void AddStrongRef(TaskSystem* Ptr);
		void SubStrongRef(TaskSystem* Ptr);
		void AddWeakRef(TaskSystem* Ptr);
		void SubWeakRef(TaskSystem* Ptr);
		bool TryAddStrongRef(TaskSystem* Ptr);
	};

	struct TaskSystemReferenceWrapper
	{
		void AddRef(TaskSystemReference* Ref) { Ref->AddWeakRef(nullptr); }
		void SubRef(TaskSystemReference* Ref) { Ref->SubWeakRef(nullptr); }
	};

	struct TaskReference
	{

		Misc::AtomicRefCount SRef;
		Misc::AtomicRefCount WRef;

		void AddStrongRef(Task* Ptr);
		void SubStrongRef(Task* Ptr);
		void AddWeakRef(Task* Ptr);
		void SubWeakRef(Task* Ptr);
		bool TryAddStrongRef(Task* Ptr);

		void AddStrongRef(TaskGraph* Ptr);
		void SubStrongRef(TaskGraph* Ptr);
		void AddWeakRef(TaskGraph* Ptr);
		void SubWeakRef(TaskGraph* Ptr);
		bool TryAddStrongRef(TaskGraph* Ptr);
	};
	*/
}



export namespace Potato::Task
{
	/*
	struct Task;
	using TaskPtr = Misc::StrongPtr<TaskSystem, TaskReference>;
	using TaskWeakPtr = Misc::WeakPtr<TaskSystem, TaskReference>;

	struct TaskGraph;
	using TaskGraphPtr = Misc::StrongPtr<TaskGraph, TaskReference>;
	using TaskGraphWeakPtr = Misc::WeakPtr<TaskGraph, TaskReference>;

	struct TaskSystem;
	
	using TaskSystemWeakPtr = Misc::WeakPtr<TaskSystem, TaskSystemReference>;
	*/


	struct TaskSystem
	{
		using Ptr = Misc::IntrusivePtr<TaskSystem>;

		void AddRef() const;
		void SubRef() const;

		static Ptr Create(std::pmr::memory_resource* MemoryPool = std::pmr::new_delete_resource(), std::pmr::memory_resource* SelfAllocator = std::pmr::new_delete_resource());

		static Ptr CreateAndFire(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1, std::pmr::memory_resource* MemoryPool = std::pmr::new_delete_resource(), std::pmr::memory_resource* SelfAllocator = std::pmr::new_delete_resource())
		{
			auto P = Create(MemoryPool, SelfAllocator);
			if (P)
			{
				P->Fire(ThreadCount);
			}
			return P;
		}

		bool Fire(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1);

	protected:

		TaskSystem(std::pmr::memory_resource* MemoryPool, std::pmr::memory_resource* SelfAllocator);
		~TaskSystem();
		static void Executor(Ptr Ptr);

		mutable Misc::AtomicRefCount RefCount;
		std::atomic_bool Avaible;
		std::pmr::memory_resource* SelfAllocator = nullptr;
		std::pmr::synchronized_pool_resource Allocator;
		std::pmr::polymorphic_allocator<std::byte> TaskAllocator;
		std::mutex ThreadMutex;
		std::vector<std::thread, std::pmr::polymorphic_allocator<std::thread>> Threads;
	};

	/*
	struct TaskGraph
	{
		struct TaskInfo
		{
			TaskWeakPtr TaskPtr;
			std::size_t Delay;
		};

		TaskGraph(TaskSystem& Owner) : OwnerPtr(Owner.GetStrongPtr()) { assert(OwnerPtr); }
		virtual TaskInfo ExportTask(TaskSystem& System) = 0;
		virtual void OnTaskExeFinish(Task& Task, TaskSystem& System) {};

	private:
		
		TaskSystemPtr OwnerPtr;
	};

	struct Task
	{
		virtual void operator()(TaskSystem& System) = 0;
		virtual ~Task() = default;
	};
	*/

	

	/*
	struct TaskGraph
	{
		virtual std::wstring_view GetGraphName() = 0;
	};




	struct TaskInterface
	{

		TaskProperty GetProperty() const { return Property;  }

	protected:

		virtual void ReleaseTask() const = 0;
		virtual void ReleaseSelf() const = 0;

		TaskInterface(TaskProperty Pro, TaskSystemPtr ) : Property(Pro) {}

	private:

		virtual bool IsReady() const { return true; }
		virtual bool IsMulityTask() const { return false; }
		virtual bool IsReCommit() const { return false; }
		virtual void Execute(TaskSystem& System) const = 0;

		void AddSRef() const;
		void AddWRef() const;
		void SubSRef() const;
		void SubWRef() const;

		mutable Misc::AtomicRefCount SRef;
		mutable Misc::AtomicRefCount WRef;

		TaskProperty const Property;

		friend struct TaskInterfaceWeakPointerWrapper;
		friend struct TaskInterfaceStrongPointerWrapper;
		friend struct TaskSystem;
	};

	struct TaskInterfaceStrongPointerWrapper
	{
		using SwitchT = TaskInterfaceWeakPointerWrapper;
		TaskInterfaceStrongPointerWrapper(TaskInterfaceWeakPointerWrapper const&) {};

		inline static void AddRef(TaskInterface const* Ptr) { assert(Ptr != nullptr); Ptr->AddSRef(); }
		inline static void SubRef(TaskInterface const* Ptr) { assert(Ptr != nullptr); Ptr->SubSRef(); }
	};

	export using TaskPtr = Misc::SmartPtr<TaskInterface, TaskInterfaceStrongPointerWrapper>;

	export struct TaskSystem
	{

		static TaskSystemPtr Create(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1, Allocator<TaskSystem> Allo= Allocator<TaskSystem>{DefaultMemoryResource()});
		
		template<typename FuncT>
		TaskPtr CreateLambdaTask(TaskProperty Property, FuncT&& Func, MemoryResource* Resource = DefaultMemoryResource());

		void AddRef();
		void SubRef();

	protected:

		~TaskSystem();
		TaskSystem(std::size_t ThreadCount, Allocator<TaskSystem> RequireAllocator);

		void Executor();

		std::atomic_bool Available = true;
		Misc::AtomicRefCount Ref;
		Allocator<TaskSystem> SelfAllocator;
		std::vector<std::thread, Allocator<std::thread>> Thread;

		friend struct TaskSystemPointerWrapper;
	};

	void TaskSystemPointerWrapper::AddRef(TaskSystem* t)
	{
		assert(t != nullptr);
		t->AddRef();
	}

	void TaskSystemPointerWrapper::SubRef(TaskSystem* t)
	{
		assert(t != nullptr);
		t->SubRef();
	}

	template<typename T>
	//requires(std::is_onvoideable_v<T, TaskSystem&>)
	struct NormalTaskImplement : public TaskInterface
	{
		template<typename ...AT>
		NormalTaskImplement(TaskProperty Property, Allocator<NormalTaskImplement<T>> Allo, AT&& ...Ats)
			: TaskInterface(Property), SelfAllocator(Allo)
		{
			CObject.emplace(std::forward<AT>(Ats)...);
		}

		virtual void ReleaseTask() const override {
			CObject.reset();
		}

		virtual void ReleaseSelf() const override {
			auto TAllo = SelfAllocator;
			this->~NormalTaskImplement();
			TAllo.deallocate_object(this, 1);
		}

		virtual void Execute(TaskSystem& System) const override {
			assert(CObject.has_value());
			(*CObject)(System);
		}

		Allocator<NormalTaskImplement<T>> SelfAllocator;
		std::optional<T> CObject;
	};

	template<typename FuncT>
	TaskPtr TaskSystem::CreateLambdaTask(TaskProperty Property, FuncT&& Func, MemoryResource* Resource)
	{
		using PureType = std::remove_reference_t<FuncT>;
		
		Allocator<NormalTaskImplement<PureType>> Allo{Resource};
		auto Ptr = Allo.allocate_object(1);
		new (Ptr) NormalTaskImplement<PureType>{
			Property, Allo, std::forward<FuncT>(Func)
		};
		TaskPtr TPtr{Ptr};
		return TPtr;
	}
	*/


}

module : private;

namespace Potato::Task
{
	auto TaskSystem::Create(std::pmr::memory_resource* MemoryPool, std::pmr::memory_resource* SelfAllocator)
		-> Ptr
	{
		assert(MemoryPool != nullptr && SelfAllocator != nullptr);
		auto AllAdress = SelfAllocator->allocate(sizeof(TaskSystem), alignof(TaskSystem));
		assert(AllAdress != nullptr);
		auto TaskPtr = new (AllAdress) TaskSystem{ MemoryPool, SelfAllocator };
		return Ptr{TaskPtr};
	}

	TaskSystem::TaskSystem(std::pmr::memory_resource* MemoryPool, std::pmr::memory_resource* SelfAllocator)
		: Avaible(true), SelfAllocator(SelfAllocator), Allocator(MemoryPool), TaskAllocator(&Allocator)
	{
	}

	bool TaskSystem::Fire(std::size_t ThreadCount)
	{
		if (ThreadMutex.try_lock())
		{
			std::lock_guard lg(ThreadMutex, std::adopt_lock_t{});
			if (Threads.empty())
			{
				auto WPtr = Ptr{this};
				Threads.reserve(ThreadCount);
				for (std::size_t I = 0; I < ThreadCount; ++I)
				{
					Threads.emplace_back(
						[WPtr]() {
							TaskSystem::Executor(std::move(WPtr));
						}
					);
				}
				return true;
			}
		}
		return false;
	}


	void TaskSystem::Executor(Ptr WP)
	{
		if (WP)
		{
			while (WP->Avaible)
			{
				std::this_thread::yield();
			}
		}
	}

	TaskSystem::~TaskSystem()
	{
		Avaible = false;

		auto Curid = std::this_thread::get_id();

		std::lock_guard lg(ThreadMutex);

		for (auto& Ite : Threads)
		{
			if (Curid == Ite.get_id())
			{
				Ite.detach();
			}
			else
				Ite.join();
		}

		Threads.clear();
	}

	void TaskSystem::AddRef() const
	{
		RefCount.AddRef();
	}
	void TaskSystem::SubRef() const
	{
		if (RefCount.SubRef())
		{
			auto OldMemopryResource = SelfAllocator;
			this->~TaskSystem();
			OldMemopryResource->deallocate(const_cast<void*>(reinterpret_cast<void const*>(this)), sizeof(TaskSystem), alignof(TaskSystem));
		}
	}

}

