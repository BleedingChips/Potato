module;

#include <cassert>

module PotatoTaskGraphic;


namespace Potato::TaskGraphic
{

	bool ContextWrapper::PauseAndPause(Task::Node& node, Task::Property property, std::optional<TimeT::time_point> delay)
	{
		return flow.PauseAndLaunch(*this, node, std::move(property), delay);
	}

	bool ContextWrapper::PauseAndPause(Node& node, Task::Property property, std::optional<TimeT::time_point> delay)
	{
		return flow.PauseAndLaunch(*this, node, std::move(property), delay);
	}

	Task::Property& ContextWrapper::GetNodeProperty()
	{
		return wrapper.GetTaskNodeProperty();
	}

	void Node::TaskExecute(Task::ContextWrapper& wrapper)
	{
		auto flow = static_cast<Flow*>(wrapper.GetTriggerProperty().trigger.GetPointer());
		ContextWrapper flow_wrapper{ wrapper, *flow};
		TaskGraphicNodeExecute(flow_wrapper);
	}

	void Node::TaskTerminal(Task::ContextWrapper& wrapper) noexcept
	{
		auto flow = static_cast<Flow*>(wrapper.GetTriggerProperty().trigger.GetPointer());
		ContextWrapper flow_wrapper{ wrapper, *flow };
		TaskGraphicNodeTerminal(flow_wrapper);
	}


	Flow::Flow(Config config)
		: config(config),
		preprocess_nodes(config.self_resource)
		, graph(config.self_resource)
		, preprocess_mutex_edges(config.self_resource)
	{

	}

	Flow::~Flow()
	{
		{
			std::lock_guard lg(preprocess_mutex);
			for(auto& ite : preprocess_nodes)
			{
				if(ite.node && !ite.under_process)
				{
					ite.node->OnLeaveFlow(*this);
				}
			}
			preprocess_nodes.clear();
			preprocess_mutex_edges.clear();
			graph.Clear();
		}
		{
			std::lock_guard lg(process_mutex);
			for(auto& ite : process_nodes)
			{
				if(ite.node)
				{
					ite.node->OnLeaveFlow(*this);
				}
			}
			process_nodes.clear();
			process_edges.clear();
			append_direct_edge.clear();
		}
	}

	bool Flow::Remove(GraphNode node)
	{
		std::lock_guard lg(preprocess_mutex);
		return Remove_AssumedLocked(node);
	}

	bool Flow::AddDirectEdge(GraphNode from, GraphNode direct_to, EdgeOptimize optimize)
	{
		std::lock_guard lg(preprocess_mutex);
		return AddDirectEdge_AssumedLocked(from, direct_to, optimize);
	}

	bool Flow::AddMutexEdge(GraphNode from, GraphNode direct_to, Graph::EdgeOptimize optimize)
	{
		std::lock_guard lg(preprocess_mutex);
		return AddMutexEdge_AssumedLocked(from, direct_to, optimize);
	}

	bool Flow::RemoveDirectEdge(GraphNode from, GraphNode direct_to)
	{
		std::lock_guard lg(preprocess_mutex);
		return RemoveDirectEdge_AssumedLocked(from, direct_to);
	}

	std::optional<std::span<GraphEdge const>> Flow::AcyclicEdgeCheck(std::span<GraphEdge> output, CheckOptimize optimize)
	{
		std::lock_guard lg(preprocess_mutex);
		return this->AcyclicEdgeCheck_AssumedLocked(output, optimize);
	}

	GraphNode Flow::AddNode(Node& node, Task::Property property)
	{
		std::lock_guard lg(preprocess_mutex);
		return AddNode_AssumedLocked(node, std::move(property));
	}

	bool Flow::AddTemporaryNode_AssumedLocked(Node& node, Task::Property property, bool(*detect_func)(void* append_data, Node& temp_node, Task::Property& temp_property, Node const& target_node, Task::Property const& target_property, FlowNodeDetectionIndex const& index), void* append_data)
	{
		if(!node.AcceptFlow(*this))
			return false;

		auto old_edge = process_edges.size();
		std::size_t cur_index = process_nodes.size();
		std::size_t init_in_degree = 0;

		if (detect_func != nullptr)
		{
			struct NodeState
			{
				std::size_t in_degree = 0;
			};

			std::pmr::vector<NodeState> state{config.temporary_resource};
			state.resize(process_nodes.size());
			std::pmr::vector<std::size_t> search_index;
			search_index.reserve(process_nodes.size());

			std::size_t index = 0;
			for (auto& ite : process_nodes)
			{
				if (ite.node && ite.state != State::DONE)
				{
					state[index].in_degree = ite.in_degree;
					if (ite.in_degree == 0)
					{
						search_index.emplace_back(index);
					}
				}
				++index;
			}

			while (!search_index.empty())
			{
				auto top = *search_index.rbegin();
				search_index.pop_back();

				auto& ref = process_nodes[top];
				assert(ref.state != State::DONE);

				if ((*detect_func)(append_data, node, property, *ref.node, ref.property, {top, temporary_node_offset }))
				{
					if (ref.state == State::READY && top < temporary_node_offset)
					{
						process_edges.push_back(top);
						ref.in_degree += 1;
					}else
					{
						ref.need_append_mutex = true;
						append_direct_edge.push_back({ top, cur_index });
						init_in_degree += 1;
					}
				}

				if (ref.state != State::READY)
				{
					auto edge = ref.direct_edges.Slice(std::span(process_edges));
					for (auto ite : edge)
					{
						assert(state[ite].in_degree >= 1);
						state[ite].in_degree -= 1;
						if (state[ite].in_degree == 0)
						{
							search_index.emplace_back(ite);
						}
					}
					if (ref.need_append_mutex)
					{
						for (auto& ite : append_direct_edge)
						{
							if (ite.from == top && ite.to < cur_index)
							{
								assert(state[ite.to].in_degree >= 1);
								state[ite.to].in_degree -= 1;
								if (state[ite.to].in_degree == 0)
								{
									search_index.push_back(ite.to);
								}
							}
						}
					}
				}
			}
		}

		process_nodes.emplace_back(
			State::READY,
			init_in_degree,
			0,
			Misc::IndexSpan<>{old_edge, process_edges.size()},
			Misc::IndexSpan<>{process_edges.size(), process_edges.size()},
			init_in_degree,
			&node,
			property,
			0,
			false
		);
		++request_task;
		return true;
	}

	bool Flow::AddTemporaryNode(Node& node, Task::Property property)
	{
		std::lock_guard lg(process_mutex);
		return this->AddTemporaryNode_AssumedLocked(node, std::move(property), nullptr, nullptr);
	}

	GraphNode Flow::AddNode_AssumedLocked(Node& node, Task::Property property)
	{
		if (node.AcceptFlow(*this))
		{
			auto t_node = graph.Add();

			if (t_node.GetIndex() < preprocess_nodes.size())
			{
				auto& ref = preprocess_nodes[t_node.GetIndex()];
				ref.node = &node;
				ref.property = property;
				ref.self = t_node;
				return t_node;
			}

			preprocess_nodes.emplace_back(&node, property, t_node);
			need_update = true;
			return t_node;
		}
		return {};
	}

	bool Flow::TryStartupNode_AssumedLock(Task::ContextWrapper& wrapper, ProcessNode& node, std::size_t index)
	{
		if (node.state == State::READY && node.mutex_degree == 0 && node.in_degree == 0)
		{
			Task::Property property = node.property;
			Task::TriggerProperty t_property;
			t_property.trigger = this;
			t_property.data2.SetIndex(index);
			auto re = wrapper.Commit(*node.node, std::move(property), std::move(t_property));
			assert(re);
			if (re)
			{
				auto mutex_span = node.mutex_edges.Slice(std::span(process_edges));
				for (auto ite : mutex_span)
				{
					process_nodes[ite].mutex_degree += 1;
				}
				node.state = State::RUNNING;
			}
		}
		return false;
	}

	bool Flow::FinishNode_AssumedLock(Task::ContextWrapper& wrapper, ProcessNode& node, std::size_t index)
	{
		if (node.state == State::RUNNING || node.state == State::PAUSE)
		{
			assert(node.pause_count == 0);
			node.state = State::DONE;

			{
				auto temp_span = std::span(process_nodes).subspan(temporary_node_offset);
				std::size_t index = temporary_node_offset;
				for (auto& ite : temp_span)
				{
					TryStartupNode_AssumedLock(wrapper, ite, index);
					++index;
				}
			}

			if (node.need_append_mutex)
			{
				for (auto& ite : append_direct_edge)
				{
					if (ite.from == index)
					{
						auto& tref = process_nodes[ite.to];
						tref.in_degree -= 1;
						TryStartupNode_AssumedLock(wrapper, tref, ite.to);
					}
				}
			}
			auto mutex_span = node.mutex_edges.Slice(std::span(process_edges));
			for (auto ite : mutex_span)
			{
				auto& tref = process_nodes[ite];
				tref.mutex_degree -= 1;
				TryStartupNode_AssumedLock(wrapper, tref, ite);
			}
			auto edge_span = node.direct_edges.Slice(std::span(process_edges));
			for (auto ite : edge_span)
			{
				auto& tref = process_nodes[ite];
				tref.in_degree -= 1;
				TryStartupNode_AssumedLock(wrapper, tref, ite);
			}

			finished_task += 1;
			if (finished_task == request_task)
			{
				{
					std::lock_guard lg(preprocess_mutex);
					switch(running_state)
					{
					case RunningState::Running:
						running_state = RunningState::Done;
						break;
					case RunningState::LockedRunning:
						running_state = RunningState::LockedDone;
						break;
					default:
						assert(false);
					}
				}
				return wrapper.Commit(*this, std::move(task_property), std::move(trigger));
			}
			return true;
		}
		else if (node.state == State::RUNNING_NEED_PAUSE)
		{
			node.state = State::PAUSE;
			return true;
		}
		return false;
	}

	bool Flow::Start(Task::ContextWrapper& wrapper)
	{
		std::lock_guard lg(process_mutex);
		if (!process_nodes.empty())
		{
			trigger = std::move(wrapper.GetTriggerProperty());
			std::size_t i = 0;
			for (auto& ite : process_nodes)
			{
				if (ite.state == State::READY)
				{
					TryStartupNode_AssumedLock(wrapper, ite, i);
				}
				++i;
			}
		}
		else
		{
			wrapper.Commit(*this, std::move(task_property), std::move(trigger));
		}
		return true;
	}

	void Flow::UpdateProcessNode()
	{
		std::lock_guard lg(preprocess_mutex);
		std::lock_guard lg2(process_mutex);

		finished_task = 0;

		auto span = std::span(process_nodes).subspan(temporary_node_offset);
		for (auto& ite : span)
		{
			ite.node->OnLeaveFlow(*this);
		}

		process_nodes.resize(temporary_node_offset);
		process_edges.resize(temporary_edge_offset);
		append_direct_edge.clear();

		if (need_update)
		{
			need_update = false;
			process_nodes.clear();
			process_edges.clear();
			real_request_task = 0;
			for (auto& ite : preprocess_nodes)
			{
				process_nodes.emplace_back(
					ite.node ? State::READY : State::DONE,
					0,
					0,
					Misc::IndexSpan<>{},
					Misc::IndexSpan<>{},
					0,
					ite.node,
					ite.property,
					0,
					false,
					ite.self
				);
				if (ite.node)
				{
					real_request_task += 1;
				}
				ite.under_process = true;
			}
			for (std::size_t i = 0; i < process_nodes.size(); ++i)
			{
				auto& ref = process_nodes[i];
				auto s = process_edges.size();
				for (auto& ite : preprocess_mutex_edges)
				{
					if (ite.from == i)
					{
						process_edges.emplace_back(ite.to);
					}
					else if (ite.to == i)
					{
						process_edges.emplace_back(ite.from);
					}
				}
				ref.mutex_edges = { s, process_edges.size() };
				s = process_edges.size();
				for (auto& ite : graph.GetEdges())
				{
					if (ite.from == i)
					{
						auto& ref = process_nodes[ite.to];
						ref.in_degree += 1;
						ref.init_in_degree += 1;
						process_edges.emplace_back(ite.to);
					}
				}
				ref.direct_edges = { s, process_edges.size() };
			}
			temporary_node_offset = process_nodes.size();
			temporary_edge_offset = process_edges.size();
			append_direct_edge.clear();
		}
		else
		{
			for (auto& ite : process_nodes)
			{
				if (ite.node)
				{
					ite.state = State::READY;
					ite.in_degree = ite.init_in_degree;
					ite.mutex_degree = 0;
					ite.need_append_mutex = false;
				}
			}
		}

		request_task = real_request_task;
	}


	constexpr std::size_t pause_offset = std::numeric_limits<std::size_t>::max() / 2;

	bool Flow::Pause_AssumedLock(ProcessNode& node)
	{
		assert(node.state == State::RUNNING || node.state == State::RUNNING_NEED_PAUSE);
		if (node.state == State::RUNNING)
			node.state = State::RUNNING_NEED_PAUSE;
		node.pause_count += 1;
		return true;
	}

	bool Flow::Continue_AssumedLock(ProcessNode& node)
	{
		assert(node.state == State::RUNNING || node.state == State::RUNNING_NEED_PAUSE || node.state == State::PAUSE);
		node.pause_count -= 1;
		if (node.pause_count == 0)
		{
			node.state = State::RUNNING;
			return true;
		}
		return false;
	}

	bool Flow::Remove_AssumedLocked(GraphNode node)
	{
		if(graph.RemoveNode(node))
		{
			auto& ref = preprocess_nodes[node.GetIndex()];
			if(!ref.under_process)
			{
				ref.node->OnLeaveFlow(*this);
			}
			ref.node.Reset();
			need_update = true;
			return true;
		}
		return false;
	}
	bool Flow::RemoveDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to)
	{
		if(graph.RemoveEdge(from, direct_to))
		{
			need_update = true;
			return true;
		}
		return false;
	}
	bool Flow::RemoveMutexEdge_AssumedLocked(GraphNode from, GraphNode direct_to)
	{
		if (graph.CheckExist(from) && graph.CheckExist(direct_to))
		{
			std::size_t o = 0;
			preprocess_mutex_edges.erase(
				std::remove_if(preprocess_mutex_edges.begin(), preprocess_mutex_edges.end(), [&](PreprocessEdge const& edge)
				{
					if (edge.from == from && edge.to == direct_to || edge.from == direct_to && edge.to == from)
					{
						o += 1;
						return true;
					}
					return false;
				}),
				preprocess_mutex_edges.end()
			);
			need_update = true;
			return o > 0;
		}
		return false;
	}
	bool Flow::AddMutexEdge_AssumedLocked(GraphNode from, GraphNode to, EdgeOptimize optimize)
	{
		if(graph.CheckExist(from) && graph.CheckExist(to))
		{
			PreprocessEdge edge{ from.GetIndex(), to.GetIndex() };
			PreprocessEdge edge2{ to.GetIndex(), from.GetIndex()};
			if(optimize.need_repeat_check)
			{
				for(auto& ite : preprocess_mutex_edges)
				{
					auto find = std::find_if(preprocess_mutex_edges.begin(), preprocess_mutex_edges.end(), [=](PreprocessEdge ite_edge)
					{
							return ite_edge == edge || ite_edge == edge2;
					});
					if(find != preprocess_mutex_edges.end())
					{
						return false;
					}
				}
			}
			preprocess_mutex_edges.emplace_back(edge);
			preprocess_mutex_edges.emplace_back(edge2);
			need_update = true;
			return true;
		}
		return false;
	}
	bool Flow::AddDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize)
	{
		if(graph.AddEdge(from, direct_to, optimize))
		{
			need_update = true;
			return true;
		}
		return false;
	}

	bool Flow::PauseAndLaunch(ContextWrapper& wrapper, Node& node, Task::Property property, std::optional<std::chrono::steady_clock::time_point> delay)
	{
		return PauseAndLaunch(wrapper, static_cast<Task::Node&>(node), std::move(property), delay);
	}

	bool Flow::PauseAndLaunch(ContextWrapper& wrapper, Task::Node& node, Task::Property property, std::optional<std::chrono::steady_clock::time_point> delay)
	{
		Task::TriggerProperty t_property = wrapper.wrapper.GetTriggerProperty();
		std::size_t index = t_property.data2.GetSizeT();
		{
			std::lock_guard lg(process_mutex);
			if (index >= pause_offset)
			{
				index = index - pause_offset;
			}
			if (!Pause_AssumedLock(process_nodes[index]))
				return false;
		}
		t_property.data2.SetIndex(index + pause_offset);
		if (wrapper.wrapper.Commit(node, std::move(property), std::move(t_property), delay))
		{
			return true;
		}
		else
		{
			std::lock_guard lg(process_mutex);
			auto re = Continue_AssumedLock(process_nodes[index]);
			assert(!re);
			return false;
		}
	}

	std::optional<std::span<Graph::GraphEdge const>> Flow::AcyclicEdgeCheck_AssumedLocked(std::span<Graph::GraphEdge> output_span, Graph::CheckOptimize optimize)
	{
		auto re = graph.AcyclicCheck(output_span, optimize);
		if (re.has_value())
		{
			need_update = false;
			return re;
		}
		else
		{
			need_update = true;
			return std::nullopt;
		}
	}

	void Flow::TaskExecute(Task::ContextWrapper& wrapper)
	{

		bool need_begin = false;

		{
			std::lock_guard lg(preprocess_mutex);

			switch (running_state)
			{
			case RunningState::Ready:
				need_begin = true;
				running_state = RunningState::Running;
				break;
			case RunningState::Locked:
				need_begin = true;
				running_state = RunningState::LockedRunning;
				break;
			case RunningState::LockedDone:
			case RunningState::Done:
				break;
			default:
				assert(false);
				return;
			}
		}

		if(need_begin)
		{
			TaskFlowExecuteBegin(wrapper);
			AcyclicEdgeCheck({}, {});
			UpdateProcessNode();
			TaskFlowPostUpdateProcessNode(wrapper);
			Start(wrapper);
		}else
		{
			TaskFlowExecuteEnd(wrapper);
			std::lock_guard lg(preprocess_mutex);
			switch (running_state)
			{
			case RunningState::Done:
				running_state = RunningState::Free;
				break;
			case RunningState::LockedDone:
				running_state = RunningState::Locked;
				break;
			default:
				assert(false);
				return;
			}
		}
	}

	bool Flow::AcceptFlow(Flow const& flow)
	{
		std::lock_guard lg(process_mutex);
		if(running_state == RunningState::Free)
		{
			running_state = RunningState::Locked;
			return true;
		}
		return false;
	}

	void Flow::OnLeaveFlow(Flow const& flow)
	{
		std::lock_guard lg(process_mutex);
		assert(running_state == RunningState::Locked);
		running_state = RunningState::Free;
	}

	void Flow::TaskTerminal(Task::ContextWrapper& wrapper) noexcept
	{
		// todo
	}

	void Flow::TriggerExecute(Task::ContextWrapper& wrapper)
	{
		std::lock_guard lg(process_mutex);
		auto trigger = wrapper.GetTriggerProperty();
		assert(trigger.data2.HasSizeT());
		auto index = trigger.data2.GetSizeT();
		if (index >= pause_offset)
		{
			index = index - pause_offset;
			assert(index < process_nodes.size());
			if (Continue_AssumedLock(process_nodes[index]))
			{
				FinishNode_AssumedLock(wrapper, process_nodes[index], index);
			}
		}
		else
		{
			assert(index < process_nodes.size());
			FinishNode_AssumedLock(wrapper, process_nodes[index], index);
		}
	}

	void Flow::TriggerTerminal(Task::ContextWrapper& wrapper) noexcept
	{
	}

	bool Flow::Commited(Task::Context& context, Task::Property property, Task::TriggerProperty trigger_property, std::optional<TimeT::time_point> delay)
	{
		std::lock_guard lg(preprocess_mutex);
		if(running_state == RunningState::Free)
		{
			if(context.Commit(*this, std::move(property), std::move(trigger_property), delay))
			{
				running_state = RunningState::Ready;
				return true;
			}
		}
		return false;
	}

	bool Flow::Commited(Task::ContextWrapper& context, Task::Property property, Task::TriggerProperty trigger_property, std::optional<TimeT::time_point> delay)
	{
		std::lock_guard lg(preprocess_mutex);
		if (running_state == RunningState::Free)
		{
			if (context.Commit(*this, std::move(property), std::move(trigger_property), delay))
			{
				running_state = RunningState::Ready;
				return true;
			}
		}
		return false;
	}


}