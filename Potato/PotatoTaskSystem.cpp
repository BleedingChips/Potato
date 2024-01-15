module;

#include <cassert>

module PotatoTaskSystem;

namespace Potato::Task
{

	TaskQueue::TaskQueue(std::pmr::memory_resource* resource)
		: time_task(resource), line_up_task(resource)
	{
		
	}

	void TaskQueue::InsertTask(Task::Ptr task, TaskProperty property)
	{
		assert(task);

		line_up_task.emplace_back(
			TaskTuple{ property, std::move(task)},
			static_cast<std::size_t>(property.priority)
		);
	}

	void TaskQueue::InsertTask(Task::Ptr task, TaskProperty property, std::chrono::system_clock::time_point time_point)
	{
		assert(task);

		min_time_point = std::min(min_time_point, time_point);

		time_task.emplace_back(
			TaskTuple{ property, std::move(task) },
			time_point
		);
	}

	bool TaskQueue::Accept(TaskProperty const& property, ThreadProperty const& thread_property, std::thread::id thread_id, bool accept_all_group)
	{
		switch (property.category)
		{
		case TaskProperty::Category::GLOBAL_TASK:
			if (thread_property.execute_global_task)
				return true;
			break;
		case TaskProperty::Category::GROUP_TASK:
			if (accept_all_group || (thread_property.execute_group_task && thread_property.group_id == property.group_id))
				return true;
			break;
		case TaskProperty::Category::THREAD_TASK:
			if(thread_id == property.thread_id)
				return true;
			break;
		}
		return false;
	}

	auto TaskQueue::PopTask(ThreadProperty property, std::thread::id thread_id, std::chrono::system_clock::time_point current_time, bool accept_all_group)
		-> std::optional<TaskTuple>
	{
		if(min_time_point < current_time)
		{
			min_time_point = std::chrono::system_clock::time_point::max();

			auto ite = time_task.begin();
			auto end = time_task.end();
			for(; ite != end; )
			{
				if(ite->time_point <= current_time)
				{
					auto priority = static_cast<std::size_t>(ite->tuple.property.priority);
					line_up_task.emplace_back(
						std::move(ite->tuple),
						priority
					);
					auto last = end - 1;
					if(ite != last)
					{
						std::swap(*ite, *last);
					}
					end = last;
				}else
				{
					min_time_point = std::min(min_time_point, ite->time_point);
					++ite;
				}
			}
			time_task.erase(end, time_task.end());
		}

		std::size_t max_priority = 0;
		auto max_ite = line_up_task.end();

		for (auto ite = line_up_task.begin(); ite != line_up_task.end(); ++ite)
		{
			if(!already_add_priority)
				ite->priority += static_cast<std::size_t>(ite->tuple.property.priority);
			if(max_priority < ite->priority 
				&& 
				Accept(ite->tuple.property, property, thread_id, accept_all_group)
				)
			{
				max_priority = ite->priority;
				max_ite = ite;
			}
		}

		already_add_priority = true;

		if(max_ite != line_up_task.end())
		{
			already_add_priority = false;
			auto next = line_up_task.end() - 1;
			if(next != max_ite)
			{
				std::swap(*next, *max_ite);
			}
			TaskTuple result = std::move(next->tuple);
			line_up_task.erase(next, line_up_task.end());
			return result;
		}
		return std::nullopt;
	}

	std::size_t TaskQueue::CheckTimeTask(ThreadProperty property, std::thread::id thread_id, bool accept_all_group)
	{
		std::size_t count = 0;
		for(auto& ite : time_task)
		{
			if(Accept(ite.tuple.property, property, thread_id, accept_all_group))
				count += 1;
		}
		return count;
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
			std::lock_guard lg(context_mutex);
			status = Status::Close;
		}


		{
			std::lock_guard lg(thread_mutex);
			for(auto& ite : thread)
			{
				ite.thread.request_stop();
			}
		}
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
		: record(record), thread(record.GetResource()), tasks(record.GetResource())
	{
		assert(record);
	}

	std::size_t TaskContext::GetSuggestThreadCount()
	{
		return std::thread::hardware_concurrency() - 1;
	}

	bool TaskContext::AddGroupThread(ThreadProperty property, std::size_t thread_count)
	{
		std::lock_guard lg(thread_mutex);
		thread.reserve(thread.size() + thread_count);
		for(std::size_t o = 0; o < thread_count; ++o)
		{
			auto ind = thread.size();

			WPtr wptr{ this };

			auto jthread = std::jthread{ [wptr = std::move(wptr), property, ind](std::stop_token token)
				{
					assert(wptr);
					wptr->ThreadExecute(std::move(token), ind, property, std::this_thread::get_id());
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

	std::size_t TaskContext::CloseThread()
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

	bool TaskContext::CommitTask(Task::Ptr task, TaskProperty property)
	{
		if(task)
		{
			std::lock_guard lg(context_mutex);
			tasks.InsertTask(std::move(task), property);
			task_count+= 1;
			return true;
		}
		return false;
	}

	bool TaskContext::CommitDelayTask(Task::Ptr task, std::chrono::system_clock::time_point time_point, TaskProperty property)
	{
		if (task)
		{
			std::lock_guard lg(context_mutex);
			tasks.InsertTask(std::move(task), property, time_point);
			task_count += 1;
			return true;
		}
		return false;
	}

	void TaskContext::ThreadExecute(std::stop_token ST, std::size_t index, ThreadProperty property, std::thread::id thread_id)
	{
		TaskQueue::TaskTuple current_task;
		Status loc_status = Status::Normal;
		bool last_execute = false;
		while(true)
		{
			auto stop_require = ST.stop_requested();

			if(context_mutex.try_lock())
			{
				std::lock_guard lg(context_mutex, std::adopt_lock);
				if(last_execute)
				{
					assert(task_count > 0);
					task_count -= 1;
					last_execute = false;
				}
				auto re = tasks.PopTask(property, thread_id, std::chrono::system_clock::now(), false);
				if(re)
				{
					current_task = std::move(*re);
				}else
				{
					if(stop_require)
					{
						auto count = tasks.CheckTimeTask(property, thread_id, false);
						if(count == 0)
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

	void TaskContext::ProcessTask(std::optional<std::size_t> override_group_id, bool flush_timer)
	{
		TaskQueue::TaskTuple current_task;
		Status loc_status = Status::Normal;
		bool last_execute = false;
		bool require_stop = false;
		ThreadProperty property
		{
			0,
			true,
			override_group_id.has_value(),
			u8"TaskContext::WaitTask"
		};
		if(override_group_id.has_value())
		{
			property.group_id = *override_group_id;
		}
		auto id = std::this_thread::get_id();
		while (true)
		{
			std::chrono::system_clock::time_point tp = std::chrono::system_clock::time_point::max();
			if (!flush_timer)
			{
				tp = std::chrono::system_clock::now();
			}

			if (context_mutex.try_lock())
			{
				std::lock_guard lg(context_mutex, std::adopt_lock);
				if (last_execute)
				{
					assert(task_count > 0);
					task_count -= 1;
					last_execute = false;
				}
				if(task_count != 0)
				{
					auto re = tasks.PopTask(property, id, tp, !override_group_id.has_value());
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