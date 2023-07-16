module;

#include <cassert>

module PotatoTaskSystem;


namespace Potato::Task
{

	struct TaskSystemImp : public TaskSystemReference
	{
		using TaskSystemReference::TaskSystemReference;

		std::optional<TaskSystem> System;
	};

	void TaskSystemReference::AddStrongRef(TaskSystem* Ptr)
	{
		SRef.AddRef();
	}
	
	void TaskSystemReference::SubStrongRef(TaskSystem* Ptr) {
		if (SRef.SubRef())
		{
			static_cast<TaskSystemImp*>(this)->System.reset();
		}
	}

	void TaskSystemReference::AddWeakRef(TaskSystem* Ptr) {
		WRef.AddRef();
	}

	void TaskSystemReference::SubWeakRef(TaskSystem* Ptr) {
		if (WRef.SubRef())
		{
			auto OldMemoryResource = RefMemoryResource;
			static_cast<TaskSystemImp*>(this)->~TaskSystemImp();
			OldMemoryResource->deallocate(this, 
				sizeof(TaskSystemImp),
				alignof(TaskSystemImp)
			);
		}
	}

	bool TaskSystemReference::TryAddStrongRef(TaskSystem* Ptr) {
		return SRef.TryAddRefNotFromZero();
	}

	auto TaskSystem::Create(std::pmr::memory_resource* MemoryResouce) -> TaskSystemPtr
	{
		assert(MemoryResouce != nullptr);

		void* APtr = MemoryResouce->allocate(
			sizeof(TaskSystemImp),
			alignof(TaskSystemImp)
		);

		try {
			
			TaskSystemImp* Ref = new (APtr) TaskSystemImp{
				MemoryResouce
			};

			try {
				Ref->System.emplace(static_cast<TaskSystemReference*>(Ref));
				return TaskSystemPtr{ &(*Ref->System), static_cast<TaskSystemReference*>(Ref) };
			}
			catch (...)
			{
				Ref->~TaskSystemImp();
				throw;
			}
		}
		catch (...)
		{
			MemoryResouce->deallocate(static_cast<std::byte*>(APtr), sizeof(TaskSystemImp) + sizeof(TaskSystemImp));
			throw;
		}
	}

	TaskSystem::TaskSystem(TaskSystemReference* Ref)
		: Available(true), Thread(&Ref->MemoryPool), Ref(Ref)
	{
		assert(Ref != nullptr);
	}

	bool TaskSystem::Fire(TaskSystemPtr Ref, std::size_t ThreadCount)
	{
		if (Ref)
		{
			std::lock_guard lg(Ref->ThreadMutex);
			if (Ref->Thread.empty())
			{
				auto WPtr = Ref.Downgrade();
				Ref->Thread.reserve(ThreadCount);
				for (std::size_t I = 0; I < ThreadCount; ++I)
				{
					Ref->Thread.emplace_back(
						[WPtr](){
							TaskSystem::Executor(std::move(WPtr));
						}
					);
				}
				return true;
			}
		}
		return false;
	}

	TaskSystem::~TaskSystem()
	{
		Available = false;

		auto Curid = std::this_thread::get_id();

		std::lock_guard lg(ThreadMutex);

		for (auto& Ite : Thread)
		{
			if (Curid == Ite.get_id())
			{
				Ite.detach();
			}else
				Ite.join();
		}

		Thread.clear();
	}

	void TaskSystem::Executor(TaskSystemWeakPtr WP)
	{

		while (true)
		{
			auto PK = WP.Upgrade();
			if (PK && PK->Available)
			{
				std::this_thread::yield();
			}else
				break;
		}
	}
}