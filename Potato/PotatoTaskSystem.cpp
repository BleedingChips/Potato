module;

#ifdef _WIN32

#endif


module Potato.TaskSystem;



namespace Potato::Task
{

	TaskSystem::Ptr TaskSystem::Create(std::size_t ThreadCount)
	{
		return new TaskSystem{ ThreadCount };
	}

	void TaskSystem::AddRef()
	{
		Ref.AddRef();
	}

	void TaskSystem::SubRef()
	{
		if (!Ref.SubRef())
		{
			delete this;
		}
	}



	TaskSystem::TaskSystem(std::size_t ThreadCount)
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