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
		: Resource(Resource), Threads(Resource), DelayTasks(Resource), ReadyTasks(Resource)
	{
		assert(Resource != nullptr);
	}

	bool TaskContext::FireThreads(std::size_t TaskCount)
	{
		std::lock_guard lg(ThreadMutex);
		if(!TimmerThread.joinable() && Threads.empty())
		{
			Threads.reserve(TaskCount);
			Ptr NewThis{this};
			WPtr ThisPtr{NewThis};
			TimmerThread = std::jthread{[ThisPtr](std::stop_token ST)
			{
				while(!ST.stop_requested())
				{
					Ptr InstanceThis{ ThisPtr };
					if(InstanceThis)
					{
						if(InstanceThis->TryPushDelayTask())
						{
							InstanceThis.Reset();
							std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
						}else
						{
							InstanceThis.Reset();
							std::this_thread::yield();
						}
					}else
						break;
				}
			}};

			for(std::size_t Count = 0; Count < TaskCount; ++Count)
			{
				Threads.emplace_back([ThisPtr](std::stop_token ST)
				{
					while(!ST.stop_requested())
					{
						Ptr InstanceThis{ThisPtr};
						if(InstanceThis)
						{
							auto [Re, Task] = InstanceThis->TryGetTopTask();
							if(Re)
							{
								if (Task)
								{
									Task->Execute(ExecuteStatus::Normal, *InstanceThis);
									InstanceThis.Reset();
									std::this_thread::yield();
								}
								else
								{
									InstanceThis.Reset();
									std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
								}
							}else
							{
								assert(!Task);
								InstanceThis.Reset();
								std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
							}
						}else
							break;
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
		TimmerThread.request_stop();
		for(auto& Ite : Threads)
		{
			Ite.request_stop();
			if(Ite.get_id() == CurID)
				Ite.detach();
		}
		Threads.clear();
		TimmerThread = std::jthread{};
		return NowTaskCount;
	}

	void TaskContext::ExecuteAndRemoveAllTask(std::pmr::memory_resource* TempResource)
	{
		std::vector<Task::Ptr, std::pmr::polymorphic_allocator<Task::Ptr>> TempTask(TempResource);
		{
			std::lock_guard lg(TaskMutex);
			EnableInsertTask = false;
			auto Span1 = std::span(DelayTasks).subspan(DelayTaskIte);
			auto Span2 = std::span(ReadyTasks).subspan(ReadyTasksIte);
			TempTask.reserve(
				Span1.size() + Span2.size()
			);
			for(auto& Ite : Span1)
				TempTask.push_back(std::move(Ite.Task));
			for(auto& Ite : Span2)
				TempTask.push_back(std::move(Ite.Task));
			DelayTasks.clear();
			DelayTaskIte = 0;
			ReadyTasks.clear();
			ReadyTasksIte = 0;
		}
		for(auto& Ite : TempTask)
		{
			Ite->Execute(ExecuteStatus::Close, *this);
			Ite.Reset();
		}
		TempTask.clear();
		{
			std::lock_guard lg(TaskMutex);
			EnableInsertTask = true;
		}
	}

	bool TaskContext::TryPushDelayTask()
	{
		if(TaskMutex.try_lock())
		{
			std::lock_guard lg(TaskMutex, std::adopt_lock);
			if (DelayTaskIte < DelayTasks.size())
			{
				auto CTime = std::chrono::system_clock::now();
				bool Move = false;
				for (; DelayTaskIte < DelayTasks.size(); ++DelayTaskIte)
				{
					auto& Cur = DelayTasks[DelayTaskIte];
					if (CTime >= Cur.DelayTimePoint)
					{
						Move = true;
						if (Cur.Task)
						{
							auto Pri = Cur.Task->GetTaskPriority();
							ReadyTaskT Task{
								0,
								std::move(Cur.Task)
							};
							auto find = std::find_if(ReadyTasks.begin() + ReadyTasksIte, ReadyTasks.end(),
								[](ReadyTaskT const& TaskT)
								{
									return TaskT.Priority > 0;
								});
							ReadyTasks.insert(find, std::move(Task));
						}
					}
					else
					{
						break;
					}
				}
				if (Move && DelayTaskIte * 2 >= DelayTasks.size())
				{
					DelayTasks.erase(DelayTasks.begin(), DelayTasks.begin() + DelayTaskIte);
					DelayTaskIte = 0;
				}
			}
			return true;
		}
		return false;
	}

	std::tuple<bool, Task::Ptr> TaskContext::TryGetTopTask()
	{
		if(TaskMutex.try_lock())
		{
			std::lock_guard lg(TaskMutex, std::adopt_lock);

			if(ReadyTasksIte < ReadyTasks.size())
			{
				auto Top = std::move(ReadyTasks[ReadyTasksIte]);
				++ReadyTasksIte;
				for(std::size_t I = ReadyTasksIte; I < ReadyTasks.size(); ++I)
				{
					assert(ReadyTasks[I].Priority >= Top.Priority);
					ReadyTasks[I].Priority -= Top.Priority;
				}
				if(ReadyTasksIte * 2 >= ReadyTasks.size())
				{
					ReadyTasks.erase(ReadyTasks.begin(), ReadyTasks.begin() + ReadyTasksIte);
					ReadyTasksIte = 0;
				}
				return {true, std::move(Top.Task)};
			}
			return {false, {}};
		}
		return {true, {}};
	}

	bool TaskContext::CommitTask(Task::Ptr TaskPtr)
	{
		if(TaskPtr)
		{
			std::lock_guard lg(TaskMutex);
			if(EnableInsertTask)
			{
				auto Priority = TaskPtr->GetTaskPriority();
				auto Find = std::find_if(ReadyTasks.begin() + ReadyTasksIte, ReadyTasks.end(),
					[Priority](ReadyTaskT const& T)
					{
						return Priority < T.Priority;
					}
				);
				ReadyTasks.insert(Find, ReadyTaskT{ Priority, std::move(TaskPtr) });
				return true;
			}
		}
		return false;
	}

	bool TaskContext::CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::time_point TimePoint)
	{
		if(TaskPtr)
		{
			std::lock_guard lg(TaskMutex);
			if(EnableInsertTask)
			{
				auto Find = std::find_if(DelayTasks.begin() + DelayTaskIte, DelayTasks.end(), 
				[TimePoint](DelayTaskT const& Task)
				{
					return Task.DelayTimePoint > TimePoint;
				});
				DelayTasks.insert(Find, DelayTaskT{ TimePoint, std::move(TaskPtr)});
				return true;
			}
		}
		return false;
	}
}