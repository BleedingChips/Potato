module;

#include <cassert>

export module PotatoTaskSystem;

export import PotatoSTD;
export import PotatoMisc;
export import PotatoSmartPtr;

namespace Potato::Task
{

	struct TaskReference
	{

		struct AdressInfo
		{
			void* Adress = nullptr;
			std::size_t Size = 0;
			std::size_t Align = 0;
			void (*Destructor)(void* Adress) = nullptr;
		};

		void AddStrongRef(void*) { RefCount.AddRef(); }
		void SubStrongRef(void*);
		void AddWeakRef(void*) { WRefCount.AddRef(); }
		void SubWeakRef(void*);
		void AddRef() { AddWeakRef(nullptr); }
		void SubRef() { SubWeakRef(nullptr); }
		bool TryAddStrongRef(void*) { return RefCount.TryAddRefNotFromZero(); }

		using Ptr = Misc::IntrusivePtr<TaskReference>;

	private:

		AdressInfo Info;

		mutable Misc::AtomicRefCount RefCount;
		mutable Misc::AtomicRefCount WRefCount;

		friend struct TaskSystem;
	};
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

	struct Context
	{

		using Ptr = Potato::Misc::StrongPtr<Context, Context>;
		using WPtr = Potato::Misc::WeakPtr<Context, Context>;

		static Ptr Create(std::size_t ThreadCount =
			std::thread::hardware_concurrency() - 1,
			std::pmr::memory_resource* MemoryPool = std::pmr::new_delete_resource(), std::pmr::memory_resource* SelfAllocator = std::pmr::new_delete_resource());

		struct Task
		{

			struct Property
			{
				std::u16string_view Name;
				std::size_t ProgressBar;
				std::size_t Trigger;
			};

			struct TaskInit
			{
				Context::Ptr Owner;
				TaskReference::Ptr Ref;
				std::size_t Priority;
				std::u16string Name;
			};

			Task(TaskInit Init)
				: Owner(std::move(Init.Owner)), Ref(std::move(Init.Ref)) {
				assert(Owner && Ref);
			}

		private:

			Context::Ptr Owner;
			TaskReference::Ptr Ref;
		};

	private:

		Context(std::pmr::memory_resource* MemoryPool, std::pmr::memory_resource* SelfAllocator)
			: SelfAllocator(SelfAllocator), MemoryPool(MemoryPool), Threads(MemoryPool)
		{
		}
		~Context() {}

		static void Executer(std::stop_token ST, WPtr P);

		void AddStrongRef(void*);
		void SubStrongRef(void*);
		void AddWeakRef(void*);
		void SubWeakRef(void*);
		bool TryAddStrongRef(void*);

		mutable Potato::Misc::AtomicRefCount RefCount;
		mutable Potato::Misc::AtomicRefCount WRefCount;

		std::mutex ThreadMutex;
		std::vector<std::jthread, std::pmr::polymorphic_allocator<std::jthread>> Threads;
		std::pmr::memory_resource* SelfAllocator;
		std::pmr::synchronized_pool_resource MemoryPool;

		friend struct Potato::Misc::StrongPtrDefaultWrapper<Context>;
		friend struct Potato::Misc::WeakPtrDefaultWrapper<Context>;
	};
}

module : private;

namespace Potato::Task
{

	void TaskReference::SubStrongRef(void*) {
		if (RefCount.SubRef())
		{
			if (Info.Adress != nullptr)
			{
				(*Info.Destructor)(Info.Adress);
				//MemoryResource->MemoryPool.deallocate(Info.Adress, Info.Size, Info.Align);
			}
		}
	}

	void TaskReference::SubWeakRef(void*) {
		if (WRefCount.SubRef())
		{
			//auto MR = std::move(MemoryResource);
			this->~TaskReference();
			//MemoryResource->MemoryPool.deallocate(this, sizeof(TaskReference), alignof(TaskReference));
		}
	}

	auto Context::Create(std::size_t ThreadCount, std::pmr::memory_resource* MemoryPool, std::pmr::memory_resource* SelfAllocator)
		-> Ptr
	{
		ThreadCount = std::max(ThreadCount, std::size_t{1});
		assert(MemoryPool != nullptr && SelfAllocator != nullptr);
		auto ConAdress = SelfAllocator->allocate(sizeof(Context), alignof(Context));
		if (ConAdress != nullptr)
		{
			auto ConPtr = new (ConAdress) Context{ MemoryPool, SelfAllocator };
			Ptr P{ ConPtr, ConPtr };
			auto WP = P.Downgrade();
			for (std::size_t I = 0; I < ThreadCount; ++I)
			{
				P->Threads.emplace_back([WP](std::stop_token ST) {
					Context::Executer(std::move(ST), std::move(WP));
					});
			}
			return P;
		}
		return {};
	}

	void Context::Executer(std::stop_token ST, WPtr P)
	{
		while (!ST.stop_requested())
		{
			auto Upgrade = P.Upgrade();
			if (Upgrade)
			{
				std::this_thread::yield();
			}
		}
	}

	void Context::AddStrongRef(void*)
	{
		RefCount.AddRef();
	}
	void Context::SubStrongRef(void*)
	{
		if (RefCount.SubRef())
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
	}

	void Context::AddWeakRef(void*)
	{
		WRefCount.AddRef();
	}
	void Context::SubWeakRef(void*)
	{
		if (WRefCount.SubRef())
		{
			auto OldSelfAllocator = SelfAllocator;
			this->~Context();
			OldSelfAllocator->deallocate(this, sizeof(Context), alignof(Context));
		}
	}

	bool Context::TryAddStrongRef(void*)
	{
		return RefCount.TryAddRefNotFromZero();
	}
}

