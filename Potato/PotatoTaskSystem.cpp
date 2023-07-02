module;

#include <cassert>

#if __INTELLISENSE__
#include "PotatoTaskSystem.ixx"
#else
module Potato.TaskSystem;
#endif

namespace Potato::Task
{


	void TaskSystemReference::AddStrongRef(TaskSystem* Ptr)
	{
		SRef.AddRef();
	}
	
	void TaskSystemReference::SubStrongRef(TaskSystem* Ptr) {
		if (SRef.SubRef())
		{
			Ptr->~TaskSystem();
		}
	}

	void TaskSystemReference::AddWeakRef(TaskSystem* Ptr) {
		WRef.AddRef();
	}

	void TaskSystemReference::SubWeakRef(TaskSystem* Ptr) {
		if (WRef.SubRef())
		{
			auto OldAllocator = Allocator;
			this->~TaskSystemReference();
			OldAllocator.deallocate_bytes(reinterpret_cast<std::byte*>(this), sizeof(TaskSystemReference) + sizeof(TaskSystem), alignof(TaskSystemReference));
		}
	}

	bool TaskSystemReference::TryAddStrongRef(TaskSystem* Ptr) {
		return SRef.TryAddRefNotFromZero();
	}


	auto TaskSystem::Create(std::size_t ThreadCount, std::pmr::memory_resource* MemoryResouce) -> Ptr
	{

		std::pmr::polymorphic_allocator<> Allocator{MemoryResouce};

		static_assert(alignof(TaskSystemReference) == alignof(TaskSystem));

		auto APtr = Allocator.allocate_bytes(sizeof(TaskSystemReference) + sizeof(TaskSystem), alignof(TaskSystemReference));

		try {
			
			TaskSystemReference* Ref = new (APtr) TaskSystemReference{
				Allocator, MemoryResouce
			};

			try {
				TaskSystem* System = new (static_cast<std::byte*>(APtr) + sizeof(TaskSystem)) TaskSystem{
					ThreadCount, MemoryResouce, Ref
				};
				return Ptr{ System, Ref };
			}
			catch (...)
			{
				Ref->~TaskSystemReference();
				throw;
			}
		}
		catch (...)
		{
			Allocator.deallocate(static_cast<std::byte*>(APtr), sizeof(TaskSystemReference) + sizeof(TaskSystem));
			throw;
		}
	}

	TaskSystem::TaskSystem(std::size_t ThreadCount, std::pmr::memory_resource* MemoryResouce, TaskSystemReference* Ref)
		: Available(true), Thread(std::pmr::polymorphic_allocator<std::thread>{MemoryResouce}), Ref(Ref)
	{
		assert(Ref != nullptr);
		ThreadCount = std::max(std::size_t{1}, ThreadCount);
		Thread.reserve(ThreadCount);
		for(std::size_t I = 0; I < ThreadCount; ++I)
			Thread.push_back(std::thread{[this](){Executor(); }});
	}

	TaskSystem::~TaskSystem()
	{
		Available = false;
		for (auto& Ite : Thread)
		{
			Ite.join();
		}
		Thread.clear();
	}

	void TaskSystem::Executor()
	{
		while (Available)
		{
			std::this_thread::yield();
		}
		volatile int i = 0;
	}
}