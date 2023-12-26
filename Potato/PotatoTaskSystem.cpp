module;

#include <cassert>

module PotatoTaskSystem;

namespace Potato::Task
{

	bool TaskQueue::Insert(Task::Ptr Task, TaskProperty Property)
	{
		if(Task)
		{
			Storage NewTask
			{
				Property.TaskPriority,
				{
					Property,
					std::move(Task)
				}
			};
			if(NewTask.IsFrontEndTask())
			{
				if(TopFrontEnd)
				{
					if(TopFrontEnd.CurrentPriority < NewTask.CurrentPriority)
					{
						std::swap(TopFrontEnd, NewTask);
					}
					AllTask.emplace_back(std::move(NewTask));
				}else
				{
					TopFrontEnd = std::move(NewTask);
				}
				
			}else
			{
				if (TopBackEnd)
				{
					if (TopBackEnd.CurrentPriority < NewTask.CurrentPriority)
					{
						std::swap(TopBackEnd, NewTask);
					}
					AllTask.emplace_back(std::move(NewTask));
				}
				else
				{
					TopBackEnd = std::move(NewTask);
				}
			}
			return true;
		}
		return false;
	}

	TaskQueue::Tuple TaskQueue::PopFrontEndTask()
	{
		if(TopFrontEnd)
		{
			auto Result = std::move(TopFrontEnd);
			auto F1 = std::find_if(AllTask.begin(), AllTask.end(), [](Storage const& Task)
			{
				return Task.IsFrontEndTask();
			});
			if(F1 != AllTask.end())
			{
				TopFrontEnd = std::move(*F1);
				std::swap(*F1, *AllTask.rbegin());
				assert(TopFrontEnd.CurrentPriority >= Result.CurrentPriority);
				TopFrontEnd.CurrentPriority -= Result.CurrentPriority;
				AllTask.pop_back();
				for(auto Ite = F1; Ite != AllTask.end(); ++Ite)
				{
					if(Ite->IsFrontEndTask())
					{
						Ite->CurrentPriority -= Result.CurrentPriority;
						if(Ite->CurrentPriority < TopFrontEnd.CurrentPriority)
						{
							std::swap(TopFrontEnd, *Ite);
						}
					}
				}
			}
			return Result.TaskTuple;
		}
		return {};
	}

	TaskQueue::Tuple TaskQueue::PopBackEndTask()
	{
		if (TopBackEnd)
		{
			auto Result = std::move(TopBackEnd);
			auto F1 = std::find_if(AllTask.begin(), AllTask.end(), [](Storage const& Task)
				{
					return !Task.IsFrontEndTask();
				});
			if (F1 != AllTask.end())
			{
				TopBackEnd = std::move(*F1);
				std::swap(*F1, *AllTask.rbegin());
				assert(TopBackEnd.CurrentPriority >= Result.CurrentPriority);
				TopBackEnd.CurrentPriority -= Result.CurrentPriority;
				AllTask.pop_back();
				for (auto Ite = F1; Ite != AllTask.end(); ++Ite)
				{
					if (!Ite->IsFrontEndTask())
					{
						Ite->CurrentPriority -= Result.CurrentPriority;
						if (Ite->CurrentPriority < TopBackEnd.CurrentPriority)
						{
							std::swap(TopBackEnd, *Ite);
						}
					}
				}
			}
			return Result.TaskTuple;
		}
		return {};
	}

	bool TimedTaskQueue::Insert(Task::Ptr Task, TaskProperty Property, std::chrono::system_clock::time_point TimePoint)
	{
		if(Task)
		{
			DelayTaskT NewTask
			{
				TimePoint,
				Property,
				std::move(Task)
			};
			if(ClosestTimePoint > TimePoint)
				ClosestTimePoint = TimePoint;
			TimedTasks.emplace_back(std::move(NewTask));
			return true;
		}
		return false;
	}

	std::size_t TimedTaskQueue::PopTask(TaskQueue& TaskQueue, std::chrono::system_clock::time_point CurrentTime)
	{
		if(CurrentTime >= ClosestTimePoint)
		{
			std::size_t Count = 0;
			ClosestTimePoint = std::chrono::system_clock::time_point::max();

			for(auto& Ite : TimedTasks)
			{
				if (Ite.DelayTimePoint <= CurrentTime)
				{
					TaskQueue.Insert(std::move(Ite.Task), Ite.Property);
					Count += 1;
				}else
				{
					if(Ite.DelayTimePoint < ClosestTimePoint)
					{
						ClosestTimePoint = Ite.DelayTimePoint;
					}
				}
			}

			if(Count != 0)
			{
				std::erase_if(
					TimedTasks,
					[](DelayTaskT const& Task)-> bool { return !Task.Task; }
				);
			}

			return Count;
		}
		return 0;
	}

	void TaskContext::ViewerRelease()
	{
		auto Rec = Record;
		assert(Rec);
		this->~TaskContext();
		Rec.Deallocate();
	}

	void TaskContext::ControllerRelease()
	{
		{
			std::lock_guard lg(TaskMutex);
			Status = TaskContextStatus::Close;
		}

		FlushTask();
		CloseThreads();
	}

	auto TaskContext::Create(std::pmr::memory_resource* Resource)->Ptr
	{
		assert(Resource != nullptr);
		auto Record = Potato::IR::MemoryResourceRecord::Allocate<TaskContext>(Resource);
		if(Record)
		{
			Ptr ptr = new (Record.Get()) TaskContext { Record };
			return ptr;
		}
		return {};
	}

	TaskContext::TaskContext(Potato::IR::MemoryResourceRecord Record)
		: Record(Record), Threads(Record.GetResource()), DelayTasks(Record.GetResource()), ReadyTasks(Record.GetResource())
	{
		assert(Record);
	}

	bool TaskContext::FireThreads(ThreadCountSetting Setting)
	{
		std::lock_guard lg(ThreadMutex);
		if(Threads.empty())
		{
			Threads.reserve(Setting.BackEndCount + Setting.DynamicBackEndCount + Setting.DynamicFrontEndCount + Setting.FrontEndCount);
			WPtr ThisPtr{this};

			for(std::size_t I = 0; I < Setting.FrontEndCount; ++I)
			{
				Threads.emplace_back([ThisPtr](std::stop_token ST)
					{
						assert(ThisPtr);
						ThisPtr->ThreadExecute(std::move(ST), ThreadType::FrontEnd);
					});
			}

			for (std::size_t I = 0; I < Setting.DynamicFrontEndCount; ++I)
			{
				Threads.emplace_back([ThisPtr](std::stop_token ST)
					{
						assert(ThisPtr);
						ThisPtr->ThreadExecute(std::move(ST), ThreadType::DynamicFrontEnd);
					});
			}

			for (std::size_t I = 0; I < Setting.DynamicBackEndCount; ++I)
			{
				Threads.emplace_back([ThisPtr](std::stop_token ST)
					{
						assert(ThisPtr);
						ThisPtr->ThreadExecute(std::move(ST), ThreadType::DynamicBackEnd);
					});
			}

			for (std::size_t I = 0; I < Setting.BackEndCount; ++I)
			{
				Threads.emplace_back([ThisPtr](std::stop_token ST)
					{
						assert(ThisPtr);
						ThisPtr->ThreadExecute(std::move(ST), ThreadType::BackEnd);
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

	bool TaskContext::CommitTask(Task::Ptr InTaskPtr, TaskProperty Property)
	{
		std::lock_guard lg(TaskMutex);
		if (ReadyTasks.Insert(std::move(InTaskPtr), Property))
		{
			++LastingTask;
			return true;
		}
		return false;
	}

	bool TaskContext::CommitDelayTask(Task::Ptr InTaskPtr, std::chrono::system_clock::time_point TimePoint, TaskProperty Property)
	{
		std::lock_guard lg(TaskMutex);
		if(DelayTasks.Insert(std::move(InTaskPtr), Property, TimePoint))
		{
			++LastingTask;
			return true;
		}
		return false;
	}

	void TaskContext::ThreadExecute(std::stop_token ST, ThreadType Type)
	{
		TaskQueue::Tuple CurrentTask;
		TaskContextStatus LocStatus = TaskContextStatus::Normal;
		bool LastExecute = false;
		while(LastExecute || !ST.stop_requested())
		{
			if(TaskMutex.try_lock())
			{
				{
					std::lock_guard lg(TaskMutex, std::adopt_lock);
					if(LastExecute)
					{
						--LastingTask;
						LastExecute = false;
						if (ST.stop_requested())
						{
							return;
						}
					}
					DelayTasks.PopTask(ReadyTasks, std::chrono::system_clock::now());
					switch(Type)
					{
					case ThreadType::FrontEnd:
						CurrentTask = ReadyTasks.PopFrontEndTask();
						break;
					case ThreadType::DynamicFrontEnd:
						CurrentTask = ReadyTasks.PopFrontEndTask();
						if(!CurrentTask.Task)
							CurrentTask = ReadyTasks.PopBackEndTask();
						break;
					case ThreadType::DynamicBackEnd:
						CurrentTask = ReadyTasks.PopBackEndTask();
						if (!CurrentTask.Task)
							CurrentTask = ReadyTasks.PopFrontEndTask();
						break;
					case ThreadType::BackEnd:
						CurrentTask = ReadyTasks.PopBackEndTask();
						break;
					}
					LocStatus = Status;
				}
				if(CurrentTask.Task)
				{
					ExecuteStatus Status{
						LocStatus,
						CurrentTask.Property,
				*this
					};
					CurrentTask.Task->operator()(Status);
					LastExecute = true;
					CurrentTask.Task.Reset();
					std::this_thread::yield();
				}else
					std::this_thread::sleep_for(std::chrono::milliseconds{1});
			}else
			{
				std::this_thread::yield();
			}
		}
	}

	void TaskContext::WaitTask()
	{
		TaskQueue::Tuple CurrentTask;
		TaskContextStatus LocStatus = TaskContextStatus::Normal;
		bool LastExecute = false;
		while(true)
		{
			if(TaskMutex.try_lock())
			{
				{
					std::lock_guard lg(TaskMutex, std::adopt_lock);
					if(LastExecute)
					{
						LastingTask -= 1;
						LastExecute = false;
					}
					DelayTasks.PopTask(ReadyTasks, std::chrono::system_clock::now());
					if(LastingTask != 0)
					{
						LocStatus = Status;
						CurrentTask = ReadyTasks.PopFrontEndTask();
						if(!CurrentTask.Task)
							CurrentTask = ReadyTasks.PopBackEndTask();
					}else
					{
						return;
					}
				}
				if(CurrentTask.Task)
				{
					ExecuteStatus Status{
						LocStatus,
						CurrentTask.Property,
				*this
					};
					CurrentTask.Task->operator()(Status);
					LastExecute = true;
					CurrentTask.Task.Reset();
				}else
					std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			}else
			{
				std::this_thread::yield();
			}
		}
	}

	void TaskContext::FlushTask(std::chrono::system_clock::time_point RequireTP)
	{
		TaskQueue::Tuple CurrentTask;
		TaskContextStatus LocStatus = TaskContextStatus::Normal;
		bool LastExecute = false;
		while (true)
		{
			if (TaskMutex.try_lock())
			{
				{
					std::lock_guard lg(TaskMutex, std::adopt_lock);
					if (LastExecute)
					{
						LastingTask -= 1;
						LastExecute = false;
					}
					DelayTasks.PopTask(ReadyTasks, RequireTP);
					if (LastingTask != 0)
					{
						LocStatus = Status;
						CurrentTask = ReadyTasks.PopFrontEndTask();
						if (!CurrentTask.Task)
							CurrentTask = ReadyTasks.PopBackEndTask();
					}
					else
					{
						return;
					}
				}
				if (CurrentTask.Task)
				{
					ExecuteStatus Status{
						LocStatus,
						CurrentTask.Property,
				*this
					};
					CurrentTask.Task->operator()(Status);
					LastExecute = true;
					CurrentTask.Task.Reset();
				}
				else
					std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}

	TaskContext::~TaskContext()
	{

		assert(Threads.empty());
	}
}