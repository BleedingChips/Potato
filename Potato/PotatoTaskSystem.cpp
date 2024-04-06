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
		-> std::tuple<std::optional<TaskTuple>, std::chrono::steady_clock::time_point>
	{

		std::size_t max_priority = 0;
		auto max_ite = line_up_task.end();

		std::chrono::steady_clock::time_point min_time_point = std::chrono::steady_clock::time_point::max();

		for (auto ite = line_up_task.begin(); ite != line_up_task.end(); ++ite)
		{
			if(ite->time_point.has_value())
			{
				if(*ite->time_point <= current_time)
					ite->time_point.reset();
				else
				{
					if((min_time_point > *ite->time_point) && Accept(ite->tuple.property, property, thread_id))
					{
						min_time_point = *ite->time_point;
					}
					continue;
				}
			}

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
			return {result, min_time_point };
		}
		return {std::nullopt, min_time_point };
	}

	TaskContext::TaskContext(std::pmr::memory_resource* resource)
		: thread(resource), line_up_task(resource)
	{
	}

	std::size_t TaskContext::GetSuggestThreadCount()
	{
		return std::thread::hardware_concurrency() - 1;
	}

	TaskContext::~TaskContext()
	{
		{
			std::lock_guard lg(execute_thread_mutex);
			status = Status::Close;
			cv.notify_all();
		}
		
		CloseAllThreadAndWait();

		while(true)
		{
			TaskTuple tup;

			{
				std::lock_guard lg(execute_thread_mutex);
				if(!line_up_task.empty())
				{
					tup = std::move(line_up_task.rbegin()->tuple);
					line_up_task.pop_back();
				}else
				{
					break;
				}
			}

			if(tup.task)
				tup.task->TaskTerminal(tup.property, tup.data);
		}
	}

	bool TaskContext::AddGroupThread(ThreadProperty property, std::size_t thread_count)
	{
		std::lock_guard lg(thread_mutex);
		thread.reserve(thread.size() + thread_count);
		for(std::size_t o = 0; o < thread_count; ++o)
		{

			auto jthread = std::jthread{ [this, property](std::stop_token token)
				{
					LineUpThreadExecute(std::move(token), property, std::this_thread::get_id());
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


	bool TaskContext::CommitTask(Task::Ptr task, TaskProperty property, AppendData data, std::u8string_view display_name)
	{
		if(task)
		{
			std::lock_guard lg(execute_thread_mutex);
			line_up_task.push_back(
				LineUpTuple{
				TaskTuple{property, std::move(task), data},
					static_cast<std::size_t>(property.priority),
					std::nullopt,
					display_name
				}
			);
			total_task_count += 1;
			cv.notify_all();
			return true;
		}
		return false;
	}

	bool TaskContext::CommitDelayTask(Task::Ptr task, std::chrono::steady_clock::time_point time_point, TaskProperty property, AppendData data, std::u8string_view display_name)
	{
		if (task)
		{
			std::lock_guard lg(execute_thread_mutex);
			line_up_task.emplace_back(
				TaskTuple{property, std::move(task), data},
				static_cast<std::size_t>(property.priority),
				time_point,
				display_name
			);
			++total_task_count;
			cv.notify_all();
			return true;
		}
		return false;
	}

	void TaskContext::LineUpThreadExecute(std::stop_token ST, ThreadProperty property, std::thread::id thread_id)
	{
		Status loc_status = Status::Normal;
		bool last_execute = false;
		while(true)
		{
			TaskTuple current_task;

			bool stop_require = false;

			{
				std::unique_lock lg(execute_thread_mutex);

				while(true)
				{
					if(last_execute)
					{
						assert(total_task_count > 0);
						total_task_count -= 1;
						last_execute = false;
					}

					stop_require = ST.stop_requested();
					loc_status = status;

					if(stop_require || loc_status == Status::Close)
						return;

					auto [re, tp] = PopLineUpTask(property, thread_id, std::chrono::steady_clock::now());

					if(re.has_value())
					{
						current_task = std::move(*re);
						break;
					}else if(tp != std::chrono::steady_clock::time_point::max())
					{
						cv.wait_until(lg, tp);
					}else
					{
						cv.wait(lg);
					}
				}
			}
			assert(current_task.task);
			if(current_task.task)
			{
				ExecuteStatus status{
					loc_status,
						*this,
						current_task.property,
						thread_id,
					property,
					current_task.data,
					current_task.display_name
				};
				current_task.task->TaskExecute(status);
				last_execute = true;
				current_task.task.Reset();
			}
		}
	}

	TaskContext::ContextStatus TaskContext::ProcessTaskOnce(ThreadProperty property, std::thread::id thread_id, std::chrono::steady_clock::time_point current)
	{
		ContextStatus re_status;
		TaskTuple tuple;
		Status loc_status = Status::Normal;
		{
			std::lock_guard lg(execute_thread_mutex);

			auto [re, exits] = PopLineUpTask(property, thread_id, current);

			if(re.has_value())
			{
				tuple = std::move(*re);
			}
			loc_status = status;
			re_status.exist_task_count = total_task_count;
			re_status.has_acceptable_task = tuple.task;
			re_status.next_waiting_task = exits;
		}

		if(tuple.task)
		{
			re_status.executed = true;
			ExecuteStatus status{
					loc_status,
						*this,
						tuple.property,
						thread_id,
					property,
				tuple.data,
				tuple.display_name
			};
			tuple.task->TaskExecute(status);

			std::lock_guard lg(execute_thread_mutex);
			total_task_count -= 1;
			re_status.exist_task_count = total_task_count;
		}
		return re_status;
	}

}