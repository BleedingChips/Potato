module;

#include <cassert>

module PotatoTaskSystem;

namespace Potato::Task
{

	bool TaskContext::Accept(TaskProperty const& property, ThreadProperty const& thread_property, std::thread::id thread_id)
	{
		switch (property.category)
		{
		case Category::GLOBAL_TASK:
			if (thread_property.global_accept != ThreadAcceptable::UnAccept)
				return true;
			break;
		case Category::GROUP_TASK:
			{
				switch(thread_property.global_accept)
				{
				case ThreadAcceptable::AcceptAll:
					return true;
				case ThreadAcceptable::SpecialAccept:
					return thread_property.group_id == property.group_id;
				default:
					return false;
				}
			}
			break;
		case Category::THREAD_TASK:
			if(thread_id == property.thread_id)
				return true;
			break;
		}
		return false;
	}

	auto TaskContext::PopLineUpTask(ThreadProperty property, std::thread::id thread_id, std::chrono::steady_clock::time_point current_time)
		-> std::optional<TaskTuple>
	{

		if(last_time_point <= current_time)
		{
			last_time_point = std::chrono::steady_clock::time_point::max();

			auto last = timed_task.end();
			for(auto ite = timed_task.begin(); ite != last;)
			{
				if(ite->time_point <= current_time)
				{
					auto priority = static_cast<std::size_t>(ite->tuple.property.priority);
					if(already_add_priority)
						priority *= 2;
					line_up_task.emplace_back(
						std::move(ite->tuple),
						priority
					);
					auto last2 = last - 1;
					if(ite != last2)
					{
						std::swap(*ite, *last2);
					}
					last = last2;
				}else
				{
					last_time_point = std::min(last_time_point, ite->time_point);
					++ite;
				}
			}
			timed_task.erase(last, timed_task.end());
		}


		std::size_t max_priority = 0;
		auto max_ite = line_up_task.end();

		for (auto ite = line_up_task.begin(); ite != line_up_task.end(); ++ite)
		{
			if (!already_add_priority)
				ite->priority += static_cast<std::size_t>(ite->tuple.property.priority);
			if (max_priority < ite->priority
				&&
				Accept(ite->tuple.property, property, thread_id)
				)
			{
				max_priority = ite->priority;
				max_ite = ite;
			}
		}

		already_add_priority = true;

		if (max_ite != line_up_task.end())
		{
			already_add_priority = false;
			auto next = line_up_task.end() - 1;
			if (next != max_ite)
			{
				std::swap(*next, *max_ite);
			}
			TaskTuple result = std::move(next->tuple);
			line_up_task.erase(next, line_up_task.end());
			return result;
		}
		return std::nullopt;
	}

	void TaskContext::ViewerRelease()
	{
		auto rec = record;
		assert(rec);
		this->~TaskContext();
		rec.Deallocate();
	}

	void TaskContext::ControllerRelease()
	{
		{
			std::lock_guard lg(execute_thread_mutex);
			status = Status::Close;
		}
		ThreadProperty property
		{
			0,
			ThreadAcceptable::AcceptAll,
			ThreadAcceptable::UnAccept,
			u8"TaskContext::ControllerRelease"
		};
		ProcessTask(property, {});
		CloseAllThreadAndWait();
	}

	auto TaskContext::Create(std::pmr::memory_resource* resource)->Ptr
	{
		assert(resource != nullptr);
		auto record = Potato::IR::MemoryResourceRecord::Allocate<TaskContext>(resource);
		if(record)
		{
			Ptr ptr = new (record.Get()) TaskContext { record };
			return ptr;
		}
		return {};
	}

	TaskContext::TaskContext(Potato::IR::MemoryResourceRecord record)
		: record(record), timed_task(record.GetResource()), thread(record.GetResource()), line_up_task(record.GetResource())
	{
		assert(record);
	}

	std::size_t TaskContext::GetSuggestThreadCount()
	{
		return std::thread::hardware_concurrency() - 1;
	}

	TaskContext::~TaskContext()
	{
		auto id = std::this_thread::get_id();

		{
			std::lock_guard lg(thread_mutex);
			while(!thread.empty())
			{
				auto top = std::move(*thread.rbegin());
				thread.pop_back();
				if(top.thread.joinable())
				{
					if (top.thread.get_id() == id)
						top.thread.detach();
					else
						top.thread.join();
				}
			}
		}
	}

	bool TaskContext::AddGroupThread(ThreadProperty property, std::size_t thread_count)
	{
		std::lock_guard lg(thread_mutex);
		thread.reserve(thread.size() + thread_count);
		for(std::size_t o = 0; o < thread_count; ++o)
		{
			WPtr wptr{ this };

			auto jthread = std::jthread{ [wptr = std::move(wptr), property](std::stop_token token)
				{
					assert(wptr);
					wptr->LineUpThreadExecute(std::move(token), property, std::this_thread::get_id());
				} };
			auto id = jthread.get_id();

			thread.emplace_back(
				property,
				id,
				std::move(jthread)
			);
		}
		return true;
	}

	std::size_t TaskContext::CloseAllThread()
	{
		std::size_t count = 0;
		std::lock_guard lg(thread_mutex);
		for(auto& ite : thread)
		{
			if(ite.thread.joinable())
			{
				ite.thread.request_stop();
				++count;
			}
		}
		return count;
	}

	std::size_t TaskContext::CloseAllThreadAndWait()
	{
		std::size_t count = 0;

		auto cid = std::this_thread::get_id();

		while(true)
		{
			ThreadCore core;

			{
				std::lock_guard lg(thread_mutex);
				if(!thread.empty())
				{
					core = std::move(*thread.rbegin());
					thread.pop_back();
					count += 1;
				}else
				{
					return count;
				}
			}

			if(core.thread.joinable())
			{
				core.thread.request_stop();
				auto tid = core.thread.get_id();
				if(tid == cid)
					core.thread.detach();
				else
					core.thread.join();
			}
		}

	}


	bool TaskContext::CommitTask(Task::Ptr task, TaskProperty property)
	{
		if(task)
		{
			std::lock_guard lg(execute_thread_mutex);
			line_up_task.push_back(
				LineUpTuple{
				TaskTuple{property, std::move(task)},
					static_cast<std::size_t>(property.priority)
				}
			);
			total_task_count += 1;
			return true;
		}
		return false;
	}

	bool TaskContext::CommitDelayTask(Task::Ptr task, std::chrono::steady_clock::time_point time_point, TaskProperty property)
	{
		if (task)
		{
			std::lock_guard lg(execute_thread_mutex);
			timed_task.emplace_back(
				TaskTuple{property, std::move(task)},
				time_point
			);
			last_time_point = std::min(last_time_point, time_point);
			++total_task_count;
			return true;
		}
		return false;
	}

	void TaskContext::LineUpThreadExecute(std::stop_token ST, ThreadProperty property, std::thread::id thread_id)
	{
		TaskTuple current_task;
		Status loc_status = Status::Normal;
		bool last_execute = false;
		while(true)
		{
			auto stop_require = ST.stop_requested();

			if(execute_thread_mutex.try_lock())
			{
				std::lock_guard lg(execute_thread_mutex, std::adopt_lock);
				loc_status = status;
				if(last_execute)
				{
					assert(total_task_count > 0);
					total_task_count -= 1;
					last_execute = false;
				}
				auto re = PopLineUpTask(property, thread_id, std::chrono::steady_clock::now());
				if(re)
				{
					current_task = std::move(*re);
				}else
				{
					if(stop_require)
					{
						bool Exist = false;
						if(thread_mutex.try_lock())
						{
							std::lock_guard lg(thread_mutex, std::adopt_lock);
							for(auto& ite : timed_task)
							{
								if(Accept(ite.tuple.property, property, thread_id))
								{
									Exist = true;
									break;
								}
							}
						}else
						{
							Exist = true;
						}
						if(!Exist)
							break;
					}

					if(loc_status == Status::Close)
					{
						if(total_task_count == 0)
							break;
					}
				}
			}else
			{
				std::this_thread::yield();
				continue;
			}
			if(current_task.task)
			{
				ExecuteStatus status{
					loc_status,
						*this,
						current_task.property,
						stop_require ? Status::Close : Status::Normal,
						thread_id,
					property
				};
				current_task.task->operator()(status);
				last_execute = true;
				current_task.task.Reset();
				std::this_thread::yield();
			}else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{1});
			}
		}
	}

	void TaskContext::ProcessTask(ThreadProperty property, bool flush_timer)
	{
		TaskTuple current_task;
		Status loc_status = Status::Normal;
		bool last_execute = false;
		bool require_stop = false;
		auto id = std::this_thread::get_id();
		while (true)
		{
			auto cur_time = std::chrono::steady_clock::time_point::max();
			if(!flush_timer)
				cur_time = std::chrono::steady_clock::now();

			if (execute_thread_mutex.try_lock())
			{
				std::lock_guard lg(execute_thread_mutex, std::adopt_lock);
				loc_status = status;
				if (last_execute)
				{
					assert(total_task_count > 0);
					total_task_count -= 1;
					last_execute = false;
				}
				if(total_task_count != 0)
				{
					auto re = PopLineUpTask(property, id, cur_time);
					if (re)
						current_task = std::move(*re);
				}else
				{
					break;
				}
			}
			else
			{
				std::this_thread::yield();
				continue;
			}
			if (current_task.task)
			{
				ExecuteStatus status{
						loc_status,
						*this,
						current_task.property,
						 Status::Normal,
						id,
					property
				};
				current_task.task->operator()(status);
				last_execute = true;
				current_task.task.Reset();
				std::this_thread::yield();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
			}
		}
	}
}