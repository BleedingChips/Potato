module;

#include <cassert>

module PotatoTask;

namespace Potato::Task
{
	NodeSequencer::NodeSequencer(Potato::IR::MemoryResourceRecord record, std::pmr::memory_resource* resource)
		: MemoryResourceRecordIntrusiveInterface(record), delay_node(resource), node_deque(resource)
	{
		
	}

	bool NodeSequencer::InsertNode(NodeTuple node, std::optional<TimeT::time_point> delay_time)
	{
		if (node.node)
		{
			std::lock_guard lg(node_deque_mutex);
			if (delay_time.has_value())
			{
				if (!min_time_point.has_value() || *delay_time < min_time_point)
					min_time_point = *delay_time;
				delay_node.emplace_back(std::move(node), *delay_time);
			}else
			{
				node_deque.emplace_back(std::move(node));
			}
			task_count += 1;
			return true;
		}
		return false;
	}

	auto NodeSequencer::PopNode(TimeT::time_point current_time) ->std::optional<NodeTuple>
	{
		std::lock_guard lg(node_deque_mutex);
		if (min_time_point.has_value() && min_time_point <= current_time)
		{
			min_time_point.reset();

			delay_node.erase(
				std::remove_if(delay_node.begin(), delay_node.end(), [&](TimedNodeTuple& ite)
				{
					if (ite.request_time < current_time)
					{
						node_deque.emplace_back(std::move(ite.node_tuple));
						return true;
					}else if (!min_time_point.has_value() || *min_time_point > ite.request_time)
					{
						min_time_point = ite.request_time;
					}
					return false;
				}),
				delay_node.end()
			);
			
		}
		if (!node_deque.empty())
		{
			auto result = std::move(*node_deque.begin());
			node_deque.pop_front();
			return std::move(result);
		}
		return std::nullopt;
	}

	auto NodeSequencer::ForcePopNode() ->std::optional<NodeTuple>
	{
		std::lock_guard lg(node_deque_mutex);
		if (min_time_point.has_value())
		{
			min_time_point.reset();
			for (auto& ite : delay_node)
			{
				node_deque.emplace_back(std::move(ite.node_tuple));
			}
			delay_node.clear();
		}
		if (!node_deque.empty())
		{
			auto result = std::move(*node_deque.rbegin());
			node_deque.pop_front();
			return std::move(result);
		}
		return std::nullopt;
	}

	auto NodeSequencer::Create(std::pmr::memory_resource* resource) -> Ptr
	{
		auto re = Potato::IR::MemoryResourceRecord::Allocate<NodeSequencer>(resource);
		if (re)
		{
			return new (re.Get()) NodeSequencer{re, re.GetMemoryResource()};
		}
		return {};
	}


	Context::Context(Config config)
		:thread_group_infos(config.self_resource), thread_infos(config.self_resource)
	{
		node_sequencer_global = NodeSequencer::Create(config.node_sequence_resource);
		node_sequencer_context = NodeSequencer::Create(config.node_sequence_resource);
	}

	std::size_t Context::AddGroupThread(std::size_t group_id, std::size_t thread_count, std::pmr::memory_resource* node_sequence_resource)
	{
		std::size_t count = 0;

		std::lock_guard lg(infos_mutex);
		auto finded = std::find_if(
			thread_group_infos.begin(),
			thread_group_infos.end(),
			[=](auto& Ite) { return Ite.group_id == group_id; }
		);

		if (finded == thread_group_infos.end())
		{
			NodeSequencer::Ptr group = NodeSequencer::Create(node_sequence_resource);
			if (!group)
				return 0;
			ThreadGroupInfo infos{
				std::move(group),
				0,
				group_id
			};
			finded = thread_group_infos.insert(thread_group_infos.end(), std::move(infos));
		}

		if (finded == thread_group_infos.end())
			return 0;

		for (std::size_t i = 0; i < thread_count; ++i)
		{
			NodeSequencer::Ptr thread_node = NodeSequencer::Create(node_sequence_resource);
			if (thread_node)
			{
				auto jthread = std::jthread{ [
					this,
					global_node = node_sequencer_global,
					group_node = finded->node_sequencer,
					thread_node = thread_node,
					group_id = group_id
				](std::stop_token token)
				{
					ThreadExecute(*group_node, *thread_node, group_id, token);
				} };

				auto id = jthread.get_id();

				thread_infos.emplace_back(
					std::move(thread_node),
					std::move(jthread),
					id,
					finded->group_id
				);

				finded->thread_count += 1;
				count += 1;
			}
		}

		return count;
	}

	Context::~Context()
	{
		{
			std::lock_guard lg(infos_mutex);
			for (auto& ite : thread_infos)
			{
				ite.thread.request_stop();
			}
		}

		while (true)
		{
			std::optional<std::jthread> target_jthread;
			{
				std::lock_guard lg(infos_mutex);
				if (!thread_infos.empty())
				{
					target_jthread = std::move(thread_infos.rbegin()->thread);
					thread_infos.pop_back();
				}
			}
			if (target_jthread.has_value())
			{
				if (target_jthread->joinable())
					target_jthread->join();
			}else
			{
				break;
			}
		}

		{
			std::lock_guard lg(infos_mutex);
			thread_group_infos.clear();
		}

		TerminalNodeSequencer(Status::ContextTerminal, *node_sequencer_global, std::numeric_limits<std::size_t>::max());
	}

	struct ContextWrapperImp : public ContextWrapper
	{
		virtual Status GetStatue() const override { return status; }
		virtual std::size_t GetGroupId() const override { return group_id; }
		virtual Property& GetTaskNodeProperty() const override { return node_tuple.property; }
		TriggerProperty& GetTriggerProperty() const override { return node_tuple.trigger; }
		virtual bool Commit(Node& target, Property property, TriggerProperty trigger = {}, std::optional<std::chrono::steady_clock::time_point> delay_time = std::nullopt) override
		{
			return context.Commit(target, std::move(property), std::move(trigger), std::move(delay_time));
		}

		virtual Node& GetCurrentTaskNode() const
		{
			return *node_tuple.node;
		}

		ContextWrapperImp(Status status, std::size_t group_id, Context& context, NodeSequencer::NodeTuple& node_tuple)
			: status(status), group_id(group_id), context(context), node_tuple(node_tuple) {}
	protected:
		Status status;
		std::size_t group_id;
		Context& context;
		NodeSequencer::NodeTuple& node_tuple;
	};

	static constexpr std::array<std::size_t, 3> task_execute_target = {4, 2, 1};

	void Context::ThreadExecute(NodeSequencer& group, NodeSequencer& thread, std::size_t group_id, std::stop_token& sk)
	{
		std::size_t count = 0;
		std::size_t test_count = 0;

		std::array<NodeSequencer*, 3> node_sequencer = { &thread, &group, node_sequencer_global.GetPointer() };
		ThreadExecuteContext ite_context;
		while (!sk.stop_requested())
		{
			TimeT::time_point now_time;
			std::size_t skip_empty_sequence_count = 0;
			std::size_t total_task_index_count = 0;
			while (total_task_index_count < 3 && skip_empty_sequence_count < 3)
			{
				if (skip_empty_sequence_count == 0)
					now_time = TimeT::now();
				auto tar = node_sequencer[ite_context.node_sequencer_selector];

				if (ExecuteNodeSequencer(*tar, group_id, now_time))
				{
					skip_empty_sequence_count = 0;
					++ite_context.current_node_sequencer_iterator;
					if (ite_context.current_node_sequencer_iterator >= task_execute_target[ite_context.node_sequencer_selector])
					{
						ite_context.current_node_sequencer_iterator = 0;
						total_task_index_count += 1;
						ite_context.node_sequencer_selector = ((ite_context.node_sequencer_selector + 1) % task_execute_target.size());
					}
				}else
				{
					skip_empty_sequence_count += 1;
					total_task_index_count += 1;
					ite_context.node_sequencer_selector = ((ite_context.node_sequencer_selector + 1) % task_execute_target.size());
				}
			}
			std::this_thread::yield();
		}

		TerminalNodeSequencer(Status::ThreadTerminal, thread, group_id);

		NodeSequencer::Ptr group_node;
		{
			std::lock_guard lg(infos_mutex);
			auto finded = std::find_if(thread_group_infos.begin(), thread_group_infos.end(), [=](ThreadGroupInfo const& info) { return info.group_id == group_id; });
			assert(finded != thread_group_infos.end());
			finded->thread_count -= 1;
			if (finded->thread_count == 0)
			{
				group_node = std::move(finded->node_sequencer);
				thread_group_infos.erase(finded);
			}
		}

		if (group_node)
		{
			TerminalNodeSequencer(Status::GroupTerminal, *group_node, group_id);
		}
	}

	bool Context::Commit(Node& target, Property property, TriggerProperty trigger, std::optional<TimeT::time_point> delay_time_point)
	{
		if (property.category.IsGlobal())
		{
			node_sequencer_global->InsertNode(
				NodeSequencer::NodeTuple{
					&target, std::move(property), std::move(trigger)
				}, std::move(delay_time_point)
			);
			return true;
		}
		if (auto group = property.category.GetGroupID(); group)
		{
			std::shared_lock sl(infos_mutex);
			auto find = std::find_if(thread_group_infos.begin(), thread_group_infos.end(), [=](ThreadGroupInfo& info) { return info.group_id == *group; });
			if (find != thread_group_infos.end())
			{
				find->node_sequencer->InsertNode(
					NodeSequencer::NodeTuple{
						&target, std::move(property), std::move(trigger)
					}, std::move(delay_time_point)
				);
				return true;
			}
			return false;
		}
		if (auto threadid = property.category.GetThreadID(); threadid)
		{
			std::shared_lock sl(infos_mutex);
			auto find = std::find_if(thread_infos.begin(), thread_infos.end(), [=](ThreadInfo& info) { return info.thread_id == *threadid; });
			if (find != thread_infos.end())
			{
				find->node_sequencer->InsertNode(
					NodeSequencer::NodeTuple{
						&target, std::move(property), std::move(trigger)
					}, std::move(delay_time_point)
				);
				return true;
			}
		}
		return false;
	}

	bool Context::ExecuteContextThreadOnce(ThreadExecuteContext& execute_context, TimeT::time_point now, bool accept_global, std::size_t group_id)
	{
		NodeSequencer* TargetSequence = nullptr;
		switch (execute_context.node_sequencer_selector)
		{
		case 0:
			TargetSequence = node_sequencer_context.GetPointer();
			break;
		case 1:
			{
				std::shared_lock sl(infos_mutex);
				auto find = std::find_if(thread_group_infos.begin(), thread_group_infos.end(), [=](ThreadGroupInfo& info) { return info.group_id == group_id; });
				if (find != thread_group_infos.end())
					TargetSequence = find->node_sequencer.GetPointer();
				break;
			}
		case 2:
			if (accept_global)
			{
				TargetSequence = node_sequencer_global.GetPointer();
				break;
			}
		}

		if (TargetSequence != nullptr && ExecuteNodeSequencer(*TargetSequence, group_id, now))
		{
			execute_context.current_node_sequencer_iterator += 1;
			if (execute_context.current_node_sequencer_iterator >= task_execute_target[execute_context.node_sequencer_selector])
			{
				execute_context.node_sequencer_selector = (execute_context.node_sequencer_selector + 1) % task_execute_target.size();
				execute_context.current_node_sequencer_iterator = 0;
				execute_context.reached_node_sequencer_count += 1;
			}
			return true;
		}
		else
		{
			if (execute_context.current_node_sequencer_iterator == 0)
				execute_context.continuous_empty_node_sequencer_count += 1;
			execute_context.node_sequencer_selector = (execute_context.node_sequencer_selector + 1) % task_execute_target.size();
			execute_context.current_node_sequencer_iterator = 0;
			execute_context.reached_node_sequencer_count += 1;
			return false;
		}
	}

	void Context::ExecuteContextThreadUntilNoExistTask(std::size_t group_id)
	{
		std::size_t loop_count = 0;
		ThreadExecuteContext context;

		TimeT::time_point now = TimeT::now();

		while (true)
		{
			
			if (!ExecuteContextThreadOnce(context, now, true, group_id))
				std::this_thread::yield();
			if (context.current_node_sequencer_iterator != 0)
				now = TimeT::now();
			if (context.current_node_sequencer_iterator == 0 && context.node_sequencer_selector == 0 && CheckNodeSequencerEmpty())
				return;
		}
	}

	bool Context::CheckNodeSequencerEmpty()
	{
		if (node_sequencer_global->GetTaskCount() != 0)
			return false;
		if (node_sequencer_context->GetTaskCount() != 0)
			return false;

		{
			std::shared_lock sl(infos_mutex);
			for (auto& ite : thread_group_infos)
			{
				if (ite.node_sequencer->GetTaskCount() != 0)
					return false;
			}
			for (auto& ite : thread_infos)
			{
				if (ite.node_sequencer->GetTaskCount() != 0)
					return false;
			}
		}

		return true;
	}

	void Context::TerminalNodeSequencer(Status state, NodeSequencer& target_sequence, std::size_t group_id) noexcept
	{
		while (true)
		{
			auto result = target_sequence.ForcePopNode();
			if (!result.has_value())
			{
				break;
			}
			ContextWrapperImp wrapper_imp{ state, group_id, *this, *result };
			result->node->TaskTerminal(wrapper_imp);
			if (result->trigger.trigger)
			{
				result->trigger.trigger->TriggerTerminal(wrapper_imp);
			}
		}
	}

	bool Context::ExecuteNodeSequencer(NodeSequencer& target_sequence, std::size_t group_id, TimeT::time_point now_time)
	{
		auto result = target_sequence.PopNode(now_time);
		if (!result.has_value())
		{
			return false;
		}
		ContextWrapperImp wrapper_imp{ Status::Normal, group_id, *this, *result };
		result->node->TaskExecute(wrapper_imp);
		if (result->trigger.trigger)
		{
			result->trigger.trigger->TriggerExecute(wrapper_imp);
		}
		target_sequence.MarkNodeFinish();
		return true;
	}
}