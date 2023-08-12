module;

#include <cassert>

module PotatoTaskSystem;

namespace Potato::Task
{
	void TaskContext::SubStrongRef()
	{
		if(SRef.SubRef())
		{
			CloseThreads();
			ExecuteAndRemoveAllTask();
		}
	}

	void TaskContext::SubWeakRef()
	{
		if (WRef.SubRef())
		{
			auto LastResource = Resource;
			assert(LastResource != nullptr);
			this->~TaskContext();
			LastResource->deallocate(this, sizeof(TaskContext), alignof(TaskContext));
		}
	}

	auto TaskContext::Create(std::pmr::memory_resource* Resource)->Ptr
	{
		assert(Resource != nullptr);
		auto MAdress = Resource->allocate(sizeof(TaskContext), alignof(TaskContext));
		if(MAdress != nullptr)
		{
			auto ContextAdress = new (MAdress) TaskContext { Resource };
			return Ptr{ ContextAdress };
		}
		return {};
	}

	TaskContext::TaskContext(std::pmr::memory_resource* Resource)
		: Resource(Resource), Threads(Resource), WaitingTask(Resource)
	{
		assert(Resource != nullptr);
	}

	bool TaskContext::FireThreads(std::size_t TaskCount)
	{
		std::lock_guard lg(ThreadMutex);
		if(Threads.empty())
		{
			Threads.reserve(TaskCount);
			Ptr NewThis{this};
			WPtr ThisPtr{NewThis};
			for(std::size_t Count = 0; Count < TaskCount; ++Count)
			{
				Threads.emplace_back([ThisPtr](std::stop_token ST)
				{
					while(!ST.stop_requested())
					{
						Ptr InstanceThis{ThisPtr};
						if(InstanceThis)
						{
							if(InstanceThis->TaskMutex.try_lock())
							{
								Task::Ptr RequireTask;
								{
									std::lock_guard lg(InstanceThis->TaskMutex, std::adopt_lock);
									if (!InstanceThis->WaitingTask.empty())
									{
										auto Top = std::move(*InstanceThis->WaitingTask.begin());
										InstanceThis->WaitingTask.pop_front();
										for(auto& Ite : InstanceThis->WaitingTask)
										{
											Ite.Priority -= Top.Priority;
										}
										RequireTask = std::move(Top.Task);
									}
								}
								if(RequireTask)
								{
									RequireTask->Execute(ExecuteStatus::Normal, *InstanceThis);
								}
								std::this_thread::sleep_for(std::chrono::nanoseconds{1});
							}else
							{
								std::this_thread::yield();
							}
						}
					}
				});
			}
		}
		return false;
	}

	std::size_t TaskContext::CloseThreads()
	{
		auto CurID = std::this_thread::get_id();
		std::lock_guard lg(ThreadMutex);
		std::size_t NowTaskCount = Threads.size();
		for(auto& Ite : Threads)
		{
			Ite.request_stop();
			if(Ite.get_id() == CurID)
				Ite.detach();
		}
		Threads.clear();
		return NowTaskCount;
	}

	void TaskContext::ExecuteAndRemoveAllTask()
	{
		bool Continue = true;
		while(Continue)
		{
			Task::Ptr RequireTask;
			{
				std::lock_guard lg(TaskMutex);
				if (!WaitingTask.empty())
				{
					auto Top = std::move(*WaitingTask.begin());
					WaitingTask.pop_front();
					for (auto& Ite : WaitingTask)
					{
						Ite.Priority -= Top.Priority;
					}
					RequireTask = std::move(Top.Task);
				}else
				{
					return;
				}
			}
			if (RequireTask)
			{
				RequireTask->Execute(ExecuteStatus::Close, *this);
			}
		}
	}

	std::tuple<bool, Task::Ptr> TaskContext::TryGetTopTask()
	{
		if(TaskMutex.try_lock())
		{
			std::lock_guard lg(TaskMutex, std::adopt_lock);
			if(StartTask >= WaitingTask.size())
			{
				return {false, {}};
			}else
			{
				auto Top = std::move(WaitingTask[StartTask]);
				++StartTask;
				for(std::size_t I = StartTask; StartTask < WaitingTask.size(); ++I)
				{
					assert(WaitingTask[I].Priority >= Top.Priority);
					WaitingTask[I].Priority -= Top.Priority;
				}
				if(StartTask * 2 >= WaitingTask.size())
				{
					WaitingTask.erase(WaitingTask.begin(), WaitingTask.begin() + StartTask);
					StartTask = 0;
				}
				return {true, std::move(Top.Task)};
			}
		}
		return {true, {}};
	}

	bool TaskContext::Commit(Task::Ptr TaskPtr, bool RequireUnique)
	{
		if(TaskPtr)
		{
			std::lock_guard lg(TaskMutex);
			if(RequireUnique)
			{
				for(auto& Ite : WaitingTask)
				{
					if(Ite.Task == TaskPtr)
					{
						return false;
					}
				}
			}
			auto Priority = TaskPtr->GetTaskPriority();
			auto Find = std::find_if(WaitingTask.begin() + StartTask, WaitingTask.end(),
			[Priority](WaittingTaskT const& T)
			{
				return Priority < T.Priority;
			}
			);
			WaitingTask.insert(Find, WaittingTaskT{Priority, std::move(TaskPtr)});
			return true;
		}
		return false;
	}

	/*
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
	*/
}