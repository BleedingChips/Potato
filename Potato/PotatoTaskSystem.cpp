module;

#include <cassert>

module PotatoTaskSystem;

namespace Potato::Task
{

	void TaskContext::Release()
	{
		auto LastResource = Resource;
		assert(LastResource != nullptr);
		this->~TaskContext();
		LastResource->deallocate(this, sizeof(TaskContext), alignof(TaskContext));
	}

	void TaskContext::ControlRelease()
	{
		{
			std::lock_guard lg(TaskMutex);
			Status = ExecuteStatus::Close;
		}

		WaitTask();
		CloseThreads();
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
		if(Threads.empty())
		{
			Threads.reserve(TaskCount);
			WPtr ThisPtr{ this };
			for(std::size_t I = 0; I < TaskCount; ++I)
			{
				Threads.emplace_back([ThisPtr](std::stop_token ST)
				{
					assert(ThisPtr);
					ExecuteContext Context;
					while(!ST.stop_requested())
					{
						ThisPtr->ExecuteOnce(Context, std::chrono::system_clock::now());
						if(Context.Locked && Context.LastingTask == 0 && Context.Status == ExecuteStatus::Close)
							break;
						if(Context.Locked && Context.LastExecute)
							std::this_thread::yield();
						else
							std::this_thread::sleep_for(std::chrono::microseconds{1});
					}
				});
			}
			return true;
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


	void TaskContext::ExecuteOnce(ExecuteContext& Context, std::chrono::system_clock::time_point CurrentTime)
	{
		Task::WPtr ExecuteOnce;
		ExecuteStatus LocStatus = ExecuteStatus::Normal;
		if(TaskMutex.try_lock())
		{
			std::lock_guard lg(TaskMutex, std::adopt_lock);
			Context.Locked = true;
			LocStatus = Status;
			if(Context.LastExecute)
			{
				Context.LastExecute = false;
				LastingTask -= 1;
			}
			Context.LastingTask = LastingTask;
			Context.Status = Status;
			if(DelayTaskIte < DelayTasks.size() && CurrentTime >= NewestTimePoint)
			{
				while(DelayTaskIte < DelayTasks.size())
				{
					auto& Top = DelayTasks[DelayTaskIte];
					if(Top.DelayTimePoint <= CurrentTime)
					{
						auto Pri = Top.Task->GetTaskPriority();
						ReadyTaskT Task{
							Pri,
							std::move(Top.Task)
						};
						DelayTaskIte += 1;
						auto find = std::find_if(ReadyTasks.begin() + ReadyTasksIte, ReadyTasks.end(),
							[](ReadyTaskT const& TaskT)
							{
								return TaskT.Priority > 0;
							});
						ReadyTasks.insert(find, std::move(Task));
					}else
					{
						NewestTimePoint = Top.DelayTimePoint;
						break;
					}
				}

				if (DelayTaskIte * 2 >= DelayTasks.size())
				{
					DelayTasks.erase(DelayTasks.begin(), DelayTasks.begin() + DelayTaskIte);
					DelayTaskIte = 0;
				}
			}
			if(ReadyTasksIte < ReadyTasks.size())
			{
				auto Top = std::move(ReadyTasks[ReadyTasksIte]);
				ExecuteOnce = std::move(Top.Task);
				++ReadyTasksIte;
				if (ReadyTasksIte * 2 >= ReadyTasks.size())
				{
					ReadyTasks.erase(ReadyTasks.begin(), ReadyTasks.begin() + ReadyTasksIte);
					ReadyTasksIte = 0;
				}
				for(std::size_t I = ReadyTasksIte; I < ReadyTasks.size(); ++I)
					ReadyTasks[I].Priority -= Top.Priority;
			}
		}else
		{
			Context.Locked = false;
		}

		if(ExecuteOnce)
		{
			ExecuteOnce->operator()(LocStatus, *this);
			Context.LastExecute = true;
		}
	}

	bool TaskContext::CommitTask(Task::Ptr InTaskPtr)
	{
		if(InTaskPtr)
		{
			Task::WPtr TaskPtr{ InTaskPtr.GetPointer() };
			std::lock_guard lg(TaskMutex);
			auto Priority = TaskPtr->GetTaskPriority();
			auto Find = std::find_if(ReadyTasks.begin() + ReadyTasksIte, ReadyTasks.end(),
				[Priority](ReadyTaskT const& T)
				{
					return Priority < T.Priority;
				}
			);
			ReadyTasks.insert(Find, ReadyTaskT{ Priority, std::move(TaskPtr) });
			++LastingTask;
			return true;
		}
		return false;
	}

	bool TaskContext::CommitDelayTask(Task::Ptr InTaskPtr, std::chrono::system_clock::time_point TimePoint)
	{
		if(InTaskPtr)
		{
			Task::WPtr TaskPtr{ InTaskPtr.GetPointer() };
			std::lock_guard lg(TaskMutex);
			auto Find = std::find_if(DelayTasks.begin() + DelayTaskIte, DelayTasks.end(),
				[TimePoint](DelayTaskT const& Task)
				{
					return Task.DelayTimePoint > TimePoint;
				});
			DelayTasks.insert(Find, DelayTaskT{ TimePoint, std::move(TaskPtr) });
			++LastingTask;
			return true;
		}
		return false;
	}

	void TaskContext::FlushTask()
	{
		ExecuteContext Context;
		while(!(Context.Locked && Context.LastingTask == 0))
		{
			ExecuteOnce(Context, std::chrono::system_clock::time_point::max());
			if (Context.Locked && Context.LastExecute)
				std::this_thread::yield();
			else
				std::this_thread::sleep_for(std::chrono::nanoseconds{ 1 });
		}
	}

	void TaskContext::WaitTask()
	{
		ExecuteContext Context;
		while (!(Context.Locked && Context.LastingTask == 0))
		{
			ExecuteOnce(Context, std::chrono::system_clock::now());
			if (Context.Locked && Context.LastExecute)
				std::this_thread::yield();
			else
				std::this_thread::sleep_for(std::chrono::nanoseconds{ 1 });
		}
	}

	TaskContext::~TaskContext()
	{
		{
			std::lock_guard lg(TaskMutex);
			Status = ExecuteStatus::Close;
		}

		auto CurID = std::this_thread::get_id();
		{
			std::lock_guard lg(ThreadMutex);
			for(auto& Ite : Threads)
			{
				Ite.request_stop();
				if(Ite.get_id() == CurID)
					Ite.detach();
			}
			Threads.clear();
		}
	}
}