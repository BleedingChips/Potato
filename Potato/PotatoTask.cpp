module;

#include <cassert>

module PotatoTask;

namespace Potato::Task
{

	Context::Context(Config config)
		:delay_node_sequencer(config.resource), node_sequencer(config.resource), thread_infos(config.resource)
	{
	}

	std::optional<std::size_t> Context::CreateThreads(std::size_t thread_count, ThreadProperty thread_property)
	{
		std::lock_guard lg(infos_mutex);
		if (current_state != Status::Normal)
		{
			return {};
		}
		for (std::size_t count = 0; count < thread_count; ++count)
		{
			auto current_thread = std::jthread{ [
					this, thread_property
				] (std::stop_token token)
				{
					ThreadExecute(thread_property, token);
				} };
			auto thread_id = current_thread.get_id();
			thread_infos.emplace_back(std::move(current_thread), std::move(thread_id), std::move(thread_property));
		}
		return thread_infos.size();
	}

	bool Context::Commit(Node& node, Node::Parameter parameter)
	{
		std::shared_lock sl(infos_mutex);
		if (current_state != Status::Normal)
		{
			return false;
		}

		if (parameter.trigger_time.has_value())
		{
			auto delay_time = *parameter.trigger_time;
			std::lock_guard lg(delay_node_sequencer_mutex);
			if (!min_time_point.has_value() || delay_time < *min_time_point)
				min_time_point = delay_time;
			delay_node_sequencer.emplace_back(
				NodeTuple{ &node, std::move(parameter) },
				delay_time
			);
		}else
		{
			std::lock_guard lg(node_sequencer_mutex);
			if (current_state != Status::Normal)
				return false;
			node_sequencer.emplace_back(&node, std::move(parameter));
			++exist_node_count;
		}
		return true;
	}

	Context::~Context()
	{
		{
			std::pmr::vector<ThreadInfo> temp_infos;
			{
				std::lock_guard lg(infos_mutex);
				current_state = Status::Terminal;
				temp_infos = std::move(thread_infos);
			}

			for (auto& ite : temp_infos)
			{
				ite.thread.request_stop();
			}

			for (auto& ite : temp_infos)
			{
				ite.thread.join();
			}

			temp_infos.clear();
		}

		{
			decltype(delay_node_sequencer) temp_delay_node;
			{
				std::lock_guard lg(delay_node_sequencer_mutex);
				temp_delay_node = std::move(delay_node_sequencer);
			}
			for (auto& ite : temp_delay_node)
			{
				ite.node_tuple.node->TaskTerminal(ite.node_tuple.parameter);
			}
		}

		{
			decltype(node_sequencer) temp_node;
			{
				std::lock_guard lg(node_sequencer_mutex);
				temp_node = std::move(node_sequencer);
			}
			for (auto& ite : temp_node)
			{
				ite.node->TaskTerminal(ite.parameter);
			}
		}
	}

	void Context::ThreadExecute(ThreadProperty thread_property, std::stop_token& sk)
	{
		ExecuteResult result;
		while (!sk.stop_requested())
		{
			ExecuteContextThreadOnce(result, thread_property);
			if (!result.has_been_execute)
			{
				std::this_thread::sleep_for(std::chrono::microseconds{10});
			}else
			{
				std::this_thread::yield();
			}
		}
		FinishExecuteContext(result);
	}

	void Context::FinishExecuteContext(ExecuteResult& result)
	{
		if (result.has_been_execute)
		{
			result.has_been_execute = false;
			std::lock_guard lg(node_sequencer_mutex);
			--exist_node_count;
			result.exist_node = exist_node_count;
		}
	}

	void Context::ExecuteContextThreadOnce(ExecuteResult& result, ThreadProperty property, TimeT::time_point now)
	{
		result.exist_delay_node.reset();
		if (delay_node_sequencer_mutex.try_lock())
		{
			std::lock_guard lg(delay_node_sequencer_mutex, std::adopt_lock);
			if (min_time_point.has_value() && min_time_point <= now)
			{
				min_time_point.reset();
				std::lock_guard lg2(node_sequencer_mutex);
				delay_node_sequencer.erase(
					std::remove_if(delay_node_sequencer.begin(), delay_node_sequencer.end(), 
						[&](TimedNodeTuple& tuple)
						{
							if (tuple.request_time < now)
							{
								node_sequencer.push_back(std::move(tuple.node_tuple));
								exist_node_count += 1;
								return true;
							}else
							{
								if (!min_time_point.has_value() || min_time_point > tuple.request_time)
								{
									min_time_point = tuple.request_time;
								}
								return false;
							}
						}
					),
					delay_node_sequencer.end()
				);
			}
			result.exist_delay_node = delay_node_sequencer.size();
		}

		NodeTuple current_node_tuple;

		{
			std::lock_guard lg(node_sequencer_mutex);
			if (result.has_been_execute)
			{
				exist_node_count -= 1;
				result.has_been_execute = false;
			}
			if (!node_sequencer.empty())
			{
				std::size_t empty_count = 0;
				for (auto& ite : node_sequencer)
				{
					if ((ite.parameter.acceptable_mask & property.acceptable_mask) != 0)
					{
						assert(ite.node);
						current_node_tuple = std::move(ite);
						ite.parameter.acceptable_mask = 0;
						break;
					}
					else if (ite.parameter.acceptable_mask == 0)
					{
						empty_count += 1;
					}
				}
				if (empty_count * 2 > node_sequencer.size())
				{
					node_sequencer.erase(
						std::remove_if(node_sequencer.begin(), node_sequencer.end(), [](NodeTuple& tuple) {
							if (tuple.parameter.acceptable_mask == 0)
							{
								assert(!tuple.node);
								return true;
							}
							return false;
						}),
						node_sequencer.end()
					);
				}
			}
			result.exist_node = exist_node_count;
		}

		if (current_node_tuple.node)
		{
			current_node_tuple.node->TaskExecute(*this, current_node_tuple.parameter);
			result.has_been_execute = true;
		}
	}

	void Context::ExecuteContextThreadUntilNoExistTask(Potato::Task::ThreadProperty property = {})
	{
		ExecuteResult result;
		while (true)
		{
			ExecuteContextThreadOnce(result, property);
			if (result.exist_delay_node.has_value() && *result.exist_delay_node == 0 && result.exist_node ==0)
			{
				break;
			}
			if (!result.has_been_execute)
			{
				std::this_thread::sleep_for(std::chrono::microseconds{ 10 });
			}
		}
		FinishExecuteContext(result);
	}
}