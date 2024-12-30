module;

#include <cassert>

module PotatoTaskGraphic;


namespace Potato::TaskGraphic
{

	FlowProcessContext::FlowProcessContext(Config config)
		: config(config), process_nodes(config.self_resource), process_edges(config.self_resource), append_direct_edge(config.self_resource)
	{
		
	}

	bool FlowProcessContext::AddTemporaryNode_AssumedLocked(Task::Node::Ptr node, Task::Property property, bool(*detect_func)(void* append_data, Task::Node const&, Task::Property const&, FlowNodeDetectionIndex const& index), void* append_data)
	{
		if (node)
		{
			auto old_edge = process_edges.size();
			std::size_t cur_index = process_nodes.size();
			std::size_t init_in_degree = 0;

			if (detect_func != nullptr)
			{
				std::vector<std::size_t> search_index;
				std::size_t index = 0;
				for (auto& ite : process_nodes)
				{
					if (ite.init_in_degree == 0)
					{
						search_index.emplace_back(index);
					}
					++index;
				}

				while (!search_index.empty())
				{
					auto top = *search_index.rbegin();
					search_index.pop_back();

					auto& ref = process_nodes[top];

					if (ref.state != State::DONE)
					{
						if ((*detect_func)(append_data, *ref.node, ref.property, { top, temporary_node_offset }))
						{
							if (top < temporary_node_offset && ref.state == State::READY)
							{
								process_edges.push_back(top);
								ref.in_degree += 1;
								continue;
							}
							else
							{
								ref.need_append_mutex = true;
								append_direct_edge.push_back({ top, cur_index });
								init_in_degree += 1;
							}
						}
					}

					if (ref.state != State::READY)
					{
						auto Misc = ref.direct_edges.Slice(std::span(process_edges));
						search_index.insert(search_index.end(), Misc.begin(), Misc.end());
						if (ref.need_append_mutex)
						{
							for (auto& ite : append_direct_edge)
							{
								if (ite.from == top)
									search_index.push_back(ite.to);
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
				std::move(node),
				property,
				0,
				false
			);
			return true;
		}
		return false;
	}

	bool FlowProcessContext::UpdateFromFlow_AssumedLocked(Flow& flow)
	{
		if (current_state == State::DONE || current_state == State::READY)
		{
			current_state = FlowProcessContext::State::READY;
			finished_task = 0;

			std::shared_lock sl(flow.preprocess_mutex);
			if (flow.update_state == Flow::UpdateState::Updated)
			{
				return false;
			}
			else if (flow.update_state == Flow::UpdateState::Bad || (flow.update_state == Flow::UpdateState::None && flow.version == version))
			{
				assert(finished_task == request_task);
				finished_task = 0;
				process_nodes.resize(temporary_node_offset);
				process_edges.resize(temporary_edge_offset);
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
				append_direct_edge.clear();
			}
			else
			{
				version = flow.version;
				process_nodes.clear();
				process_edges.clear();
				request_task = 0;
				for (auto& ite : flow.preprocess_nodes)
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
						false
					);
					if (ite.node)
					{
						request_task += 1;
					}
				}
				for (std::size_t i = 0; i < process_nodes.size(); ++i)
				{
					auto& ref = process_nodes[i];
					auto s = process_edges.size();
					for (auto& ite : flow.preprocess_mutex_edges)
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
					for (auto& ite : flow.graph.GetEdges())
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
			return true;
		}
		return false;
	}

	FlowProcessContext::~FlowProcessContext()
	{
		std::lock_guard lg(process_mutex);
		process_nodes.clear();
	}

	bool FlowProcessContext::TryStartupNode_AssumedLock(Flow& flow, Task::ContextWrapper& wrapper, ProcessNode& node, std::size_t index)
	{
		if (node.state == State::READY && node.mutex_degree == 0 && node.in_degree == 0)
		{
			Task::Property property = node.property;
			Task::TriggerProperty t_property;
			t_property.trigger = &flow;
			t_property.data.SetNodeData(this);
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

	bool FlowProcessContext::FinishNode_AssumedLock(Flow& flow, Task::ContextWrapper& wrapper, ProcessNode& node, std::size_t index)
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
					TryStartupNode_AssumedLock(flow, wrapper, ite, index);
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
						TryStartupNode_AssumedLock(flow, wrapper, tref, ite.to);
					}
				}
			}
			auto mutex_span = node.mutex_edges.Slice(std::span(process_edges));
			for (auto ite : mutex_span)
			{
				auto& tref = process_nodes[ite];
				tref.mutex_degree -= 1;
				TryStartupNode_AssumedLock(flow, wrapper, tref, ite);
			}
			auto edge_span = node.direct_edges.Slice(std::span(process_edges));
			for (auto ite : edge_span)
			{
				auto& tref = process_nodes[ite];
				tref.in_degree -= 1;
				TryStartupNode_AssumedLock(flow, wrapper, tref, ite);
			}

			finished_task += 1;
			if (finished_task == request_task)
			{
				auto trigger_property = wrapper.GetTriggerProperty();
				Task::Property property;
				property.data.SetNodeData(this);
				property.data2.SetIndex(1);
				wrapper.Commit(flow, std::move(property), std::move(trigger));
				current_state = State::DONE;
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

	bool FlowProcessContext::Start(Flow& flow, Task::ContextWrapper& wrapper)
	{
		std::lock_guard lg(process_mutex);
		if (UpdateFromFlow_AssumedLocked(flow))
		{
			if (!process_nodes.empty())
			{
				trigger = std::move(wrapper.GetTriggerProperty());
				std::size_t i = 0;
				for (auto& ite : process_nodes)
				{
					if (ite.state == State::READY)
					{
						TryStartupNode_AssumedLock(flow, wrapper, ite, i);
					}
					++i;
				}
			}else
			{
				auto trigger_property = wrapper.GetTriggerProperty();
				Task::Property property;
				property.data.SetNodeData(this);
				property.data2.SetIndex(1);
				wrapper.Commit(flow, std::move(property), std::move(trigger_property));
				current_state = State::DONE;
			}
			return true;
		}
		return false;
	}

	constexpr std::size_t pause_offset = std::numeric_limits<std::size_t>::max() / 2;

	bool FlowProcessContext::OnTaskFlowFlowTriggerExecute(Flow& flow, Task::ContextWrapper& wrapper)
	{
		std::lock_guard lg(process_mutex);
		auto trigger = wrapper.GetTriggerProperty();
		assert(trigger.data.HasNodeDataPointer() && trigger.data2.HasSizeT());
		auto index = trigger.data2.GetSizeT();
		if (index >= pause_offset)
		{
			index = index - pause_offset;
			assert(index < process_nodes.size());
			if (Continue_AssumedLock(process_nodes[index]))
			{
				return FinishNode_AssumedLock(flow, wrapper, process_nodes[index], index);
			}
		}else
		{
			assert(index < process_nodes.size());
			return FinishNode_AssumedLock(flow, wrapper, process_nodes[index], index);
		}
		return true;
	}

	bool FlowProcessContext::PauseAndLaunch(Task::ContextWrapper& wrapper, Flow& flow, std::size_t index, Task::Node& ptr, Task::Property property, std::optional<std::chrono::steady_clock::time_point> delay_time)
	{
		{
			std::lock_guard lg(process_mutex);
			if (index >= pause_offset)
			{
				index = index - pause_offset;
			}
			if (!Pause_AssumedLock(process_nodes[index]))
				return false;
		}

		assert(
			dynamic_cast<Flow*>(wrapper.GetTriggerProperty().trigger.GetPointer()) == &flow
			&&
			wrapper.GetTriggerProperty().data.GetNodeDataPointer().GetPointer() == this
		);

		Task::TriggerProperty t_property = wrapper.GetTriggerProperty();
		t_property.data2.SetIndex(index + pause_offset);
		if (wrapper.Commit(ptr, std::move(property), std::move(t_property), std::move(delay_time)))
		{
			return true;
		}else
		{
			std::lock_guard lg(process_mutex);
			auto re = Continue_AssumedLock(process_nodes[index]);
			assert(!re);
			return false;
		}
	}

	bool FlowProcessContext::Pause_AssumedLock(ProcessNode& node)
	{
		assert(node.state == State::RUNNING || node.state == State::RUNNING_NEED_PAUSE);
		if (node.state == State::RUNNING)
			node.state = State::RUNNING_NEED_PAUSE;
		node.pause_count += 1;
		return true;
	}

	bool FlowProcessContext::Continue_AssumedLock(ProcessNode& node)
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
			preprocess_nodes.clear();
			preprocess_mutex_edges.clear();
			graph.Clear();
		}
	}

	GraphNode Flow::AddNode_AssumedLocked(Task::Node::Ptr node, Task::Property property)
	{
		if(node)
		{
			auto t_node = graph.Add();

			if (t_node.GetIndex() < preprocess_nodes.size())
			{
				auto& ref = preprocess_nodes[t_node.GetIndex()];
				ref.node = std::move(node);
				ref.property = property;
				ref.self = t_node;
				return t_node;
			}

			preprocess_nodes.emplace_back(std::move(node), property, t_node);
			update_state = UpdateState::Updated;
			return t_node;
		}
		return {};
	}


	bool Flow::Remove_AssumedLocked(GraphNode node)
	{
		if(graph.RemoveNode(node))
		{
			preprocess_nodes[node.GetIndex()].node.Reset();
			update_state = UpdateState::Updated;
			return true;
		}
		return false;
	}
	bool Flow::RemoveDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to)
	{
		if(graph.RemoveEdge(from, direct_to))
		{
			update_state = UpdateState::Updated;
			return true;
		}
		return false;
	}
	bool Flow::AddMutexEdge_AssumedLocked(GraphNode from, GraphNode to, EdgeOptimize optimize)
	{
		if(graph.CheckExist(from) && graph.CheckExist(to))
		{
			PreprocessEdge edge{ from.GetIndex(), to.GetIndex() };
			if(optimize.need_repeat_check)
			{
				for(auto& ite : preprocess_mutex_edges)
				{
					auto find = std::find(preprocess_mutex_edges.begin(), preprocess_mutex_edges.end(), edge);
					if(find != preprocess_mutex_edges.end())
					{
						return false;
					}
				}
			}
			preprocess_mutex_edges.emplace_back(edge);
			update_state = UpdateState::Updated;
			return true;
		}
		return false;
	}
	bool Flow::AddDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize)
	{
		if(graph.AddEdge(from, direct_to, optimize))
		{
			update_state = UpdateState::Updated;
			return true;
		}
		return false;
	}

	struct DefaultFlowProcessContext : public FlowProcessContext, public IR::MemoryResourceRecordIntrusiveInterface
	{
		DefaultFlowProcessContext(Potato::IR::MemoryResourceRecord record, FlowProcessContext::Config config)
			: FlowProcessContext(config), MemoryResourceRecordIntrusiveInterface(record) {}
	protected:
		void AddTaskGraphicFlowProcessContextRef() const override { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		void SubTaskGraphicFlowProcessContextRef() const override { MemoryResourceRecordIntrusiveInterface::SubRef(); }
	};

	FlowProcessContext::Ptr Flow::CreateProcessContext()
	{
		auto re = Potato::IR::MemoryResourceRecord::Allocate<DefaultFlowProcessContext>(config.self_resource);
		if (re)
		{
			return new(re.Get()) DefaultFlowProcessContext{std::move(re), {}};
		}
		return nullptr;
	}

	bool Flow::PauseAndLaunch(Task::ContextWrapper& wrapper, Node& node, Task::Property property, std::optional<std::chrono::steady_clock::time_point> delay)
	{
		auto& trigger = wrapper.GetTriggerProperty();
		auto flow = dynamic_cast<Flow*>(trigger.trigger.GetPointer());
		if (flow != nullptr)
		{
			auto context = trigger.data.TryGetNodeDataPointerWithType<FlowProcessContext>();
			if (context != nullptr)
			{
				return context->PauseAndLaunch(wrapper, *flow, trigger.data2.GetSizeT(), node, std::move(property), std::move(delay));
			}
		}
		return false;
	}

	void Flow::TaskExecute(Task::ContextWrapper& wrapper)
	{
		auto& property = wrapper.GetTaskNodeProperty();
		assert(property.data.HasNodeDataPointer());
		assert(property.data2.HasSizeT());
		auto process_context = property.data.TryGetNodeDataPointerWithType<FlowProcessContext>();
		assert(process_context != nullptr);

		auto value = property.data2.GetSizeT();

		if (value == 0)
		{
			TaskFlowExecuteBegin(wrapper);
			AcyclicEdgeCheck({}, {});
			if (!process_context->Start(*this, wrapper))
			{
				auto context = CreateProcessContext();
				if (context)
				{
					wrapper.GetTaskNodeProperty().data.SetNodeData(context.GetPointer());
					auto re = context->Start(*this, wrapper);
					if (!re)
					{
						TaskFlowExecuteEnd(wrapper);
					}
				}else
				{
					TaskFlowExecuteEnd(wrapper);
				}
			}
		}else  if (value == 1)
		{
			TaskFlowExecuteEnd(wrapper);
		}
	}

	void Flow::TaskTerminal(Task::ContextWrapper& wrapper) noexcept
	{
		auto& property = wrapper.GetTaskNodeProperty();
		auto process_context = property.data.TryGetNodeDataPointerWithType<FlowProcessContext>();
		assert(process_context != nullptr);
		if (process_context != nullptr)
		{
			process_context->OnTaskFlowTerminal(*this, wrapper);
		}
	}

	void Flow::TriggerExecute(Task::ContextWrapper& wrapper)
	{
		auto& property = wrapper.GetTriggerProperty();
		auto process_context = property.data.TryGetNodeDataPointerWithType<FlowProcessContext>();
		assert(process_context != nullptr);
		if (process_context != nullptr)
		{
			process_context->OnTaskFlowFlowTriggerExecute(*this, wrapper);
		}
	}

	void Flow::TriggerTerminal(Task::ContextWrapper& wrapper) noexcept
	{
		auto& property = wrapper.GetTriggerProperty();
		auto process_context = property.data.TryGetNodeDataPointerWithType<FlowProcessContext>();
		assert(process_context != nullptr);
		if (process_context != nullptr)
		{
			process_context->OnTaskFlowFlowTriggerTerminal(*this, wrapper);
		}
	}

	bool Flow::Commited(Task::Context& context, std::u8string_view display_name, Task::Catgegory catrgory)
	{
		auto execute = CreateProcessContext();
		if (execute)
		{
			Task::Property property;
			property.node_name = display_name;
			property.category = catrgory;
			property.data.SetNodeData(execute.GetPointer());
			property.data2.SetIndex(0);
			return context.Commit(*this, std::move(property));
		}
		return false;
	}

	GraphNode Flow::AddFlow(Flow& flow, std::u8string_view display_name, Task::Catgegory catrgory)
	{
		auto execute = CreateProcessContext();
		if (execute)
		{
			Task::Property property;
			property.node_name = display_name;
			property.category = catrgory;
			property.data.SetNodeData(execute.GetPointer());
			property.data2.SetIndex(0);
			return AddNode(&flow, std::move(property));
		}
		return {};
	}
}