module;

#if __INTELLISENSE__
#include "PotatoTaskSystem.ixx"
#else
module Potato.TaskSystem;
#endif

namespace Potato::Task
{

	TaskSystemPtr TaskSystem::Create(std::size_t ThreadCount, Allocator<TaskSystem> Allo)
	{
		TaskSystem* APtr = Allo.allocate_object<TaskSystem>(1);
		new (APtr) TaskSystem{ ThreadCount, Allo };
		return TaskSystemPtr{ APtr };
	}

	void TaskSystem::AddRef()
	{
		Ref.AddRef();
	}

	void TaskSystem::SubRef()
	{
		if (!Ref.SubRef())
		{
			auto TempAllo = SelfAllocator;
			this->~TaskSystem();
			TempAllo.deallocate_object<TaskSystem>(this, 1);
		}
	}

	TaskSystem::TaskSystem(std::size_t ThreadCount, Allocator<TaskSystem> InputAllocator)
		: Available(true), SelfAllocator(InputAllocator), Thread(InputAllocator)
	{
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