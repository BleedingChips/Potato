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
			Status = TaskContextStatus::Close;
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
						if(Context.Locked && Context.LastingTask == 0 && Context.Status == TaskContextStatus::Close)
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
		ReadyTaskT CurrentTask;
		TaskContextStatus LocStatus = TaskContextStatus::Normal;
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
			if(!DelayTasks.empty() && CurrentTime >= ClosestTimePoint)
			{
				ClosestTimePoint = std::chrono::system_clock::time_point::max();
				std::size_t OldReadyTaskSize = ReadyTasks.size();
				auto EndIte = DelayTasks.end();
				for(auto Ite = DelayTasks.begin(); Ite != EndIte;)
				{
					if(Ite->DelayTimePoint <= CurrentTime)
					{
						auto Cur = std::move(*Ite);
						auto& Ref = ReadyTasks.emplace_back(
							Cur.Property,
							std::move(Cur.Task)
						);
						EndIte -= 1;
						if(Ite != EndIte)
						{
							std::swap(*Ite, *EndIte);
						}
					}else if(Ite->DelayTimePoint < ClosestTimePoint)
					{
						ClosestTimePoint = Ite->DelayTimePoint;
						++Ite;
					}
				}
				DelayTasks.erase(EndIte, DelayTasks.end());
				if(OldReadyTaskSize < ReadyTasks.size())
				{
					auto Index = std::max(OldReadyTaskSize, std::size_t{1});
					auto Begin = ReadyTasks.begin();
					for(auto Ite = ReadyTasks.begin() + Index; Ite < ReadyTasks.end(); ++Ite)
					{
						if(Ite->Property.TaskPriority < Begin->Property.TaskPriority)
						{
							std::swap(*Ite, *Begin);
						}
					}
				}
			}
			if(!ReadyTasks.empty())
			{
				auto Top = std::move(*ReadyTasks.begin());
				if(ReadyTasks.size() >= 2)
				{
					auto BeginIte = ReadyTasks.begin();
					auto EndIte = ReadyTasks.end() - 1;
					std::swap(*BeginIte, *EndIte);
					BeginIte->Property.TaskPriority -= Top.Property.TaskPriority;
					for(auto Ite = BeginIte + 1; Ite < EndIte; ++Ite)
					{
						Ite->Property.TaskPriority -= Top.Property.TaskPriority;
						if(Ite->Property.TaskPriority < BeginIte->Property.TaskPriority)
						{
							std::swap(*Ite, *BeginIte);
						}
					}
					ReadyTasks.resize(ReadyTasks.size() - 1);
				}else
				{
					ReadyTasks.clear();
				}
				CurrentTask = std::move(Top);
				
			}
		}else
		{
			Context.Locked = false;
		}

		if(CurrentTask.Task)
		{
			ExecuteStatus Status{
				LocStatus,
				CurrentTask.Property,
				*this
			};
			CurrentTask.Task->operator()(Status);
			Context.LastExecute = true;
		}
	}

	bool TaskContext::CommitTask(Task::Ptr InTaskPtr, TaskProperty Property)
	{
		if(InTaskPtr)
		{
			Task::WPtr TaskPtr{ InTaskPtr.GetPointer() };
			std::lock_guard lg(TaskMutex);

			ReadyTasks.emplace_back(Property, std::move(TaskPtr));
			
			if(ReadyTasks.size() >= 2)
			{
				auto Ite1 = ReadyTasks.begin();
				auto Ite2 = ReadyTasks.rbegin();
				if(Ite1->Property.TaskPriority > Ite2->Property.TaskPriority)
					std::swap(*Ite1, *Ite2);
			}
			++LastingTask;
			return true;
		}
		return false;
	}

	bool TaskContext::CommitDelayTask(Task::Ptr InTaskPtr, std::chrono::system_clock::time_point TimePoint, TaskProperty Property)
	{
		if(InTaskPtr)
		{
			Task::WPtr TaskPtr{ InTaskPtr.GetPointer() };
			std::lock_guard lg(TaskMutex);
			if(ClosestTimePoint > TimePoint)
				ClosestTimePoint = TimePoint;
			DelayTasks.emplace_back(TimePoint, Property, std::move(TaskPtr));
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
			Status = TaskContextStatus::Close;
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