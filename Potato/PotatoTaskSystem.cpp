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
			if(!DelayTasks.empty() && CurrentTime >= ClosestTimePoint)
			{
				ClosestTimePoint = std::chrono::system_clock::time_point::max();
				std::size_t OldReadyTaskSize = ReadyTasks.size();
				auto EndIte = DelayTasks.end();
				for(auto Ite = DelayTasks.begin(); Ite != EndIte;)
				{
					if(Ite->DelayTimePoint <= CurrentTime)
					{
						std::size_t Priority = Ite->Task->GetTaskPriority();
						auto& Ref = ReadyTasks.emplace_back(
							Priority,
							std::move(Ite->Task)
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
						if(Ite->Priority < Begin->Priority)
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
					BeginIte->Priority -= Top.Priority;
					for(auto Ite = BeginIte + 1; Ite < EndIte; ++Ite)
					{
						Ite->Priority -= Top.Priority;
						if(Ite->Priority < BeginIte->Priority)
						{
							std::swap(*Ite, *BeginIte);
						}
					}
					ReadyTasks.resize(ReadyTasks.size() - 1);
				}else
				{
					ReadyTasks.clear();
				}
				ExecuteOnce = std::move(Top.Task);
				
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

			ReadyTasks.emplace_back(Priority, std::move(TaskPtr));
			
			if(ReadyTasks.size() >= 2)
			{
				auto Ite1 = ReadyTasks.begin();
				auto Ite2 = ReadyTasks.rbegin();
				if(Ite1->Priority > Ite2->Priority)
					std::swap(*Ite1, *Ite2);
			}
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
			if(ClosestTimePoint > TimePoint)
				ClosestTimePoint = TimePoint;
			DelayTasks.emplace_back(TimePoint, std::move(TaskPtr));
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