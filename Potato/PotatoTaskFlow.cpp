module;

#include <cassert>

module PotatoTaskFlow;


namespace Potato::TaskFlow
{
	Flow::Flow(std::pmr::memory_resource* resource)
		: node_infos(resource), node_direct_edges(resource),
	node_mutex_edges(resource), encoded_flow(resource)
	{
		
	}

	Flow::NodeIndex Flow::AddFlowAsNode(Flow const& flow, std::u8string_view sub_flow_name, std::pmr::memory_resource* temporary_resource)
	{
		auto old_node_size = encoded_flow.encode_infos.size();
		auto old_edge_size = encoded_flow.edges.size();

		if (EncodeNodeTo(flow, encoded_flow, temporary_resource))
		{
			std::size_t index = 0;

			NodeInfos info;
			info.category = NodeCategory::SubFlowNode;
			info.version = 1;
			info.encode_nodes = { old_node_size, encoded_flow.encode_infos.size() };
			info.encode_edges = { old_edge_size, encoded_flow.edges.size() };
			info.parameter.node_name = sub_flow_name;

			for (index = 0; index < node_infos.size(); ++index)
			{
				auto& ref = node_infos[index];
				if (ref.category == NodeCategory::Empty)
				{
					info.version = ref.version + 1;
					ref = std::move(info);
					return { index, ref.version };
				}
			}

			node_infos.emplace_back(std::move(info));

			return { index, info.version };
		}

		return {};
	}

	Flow::NodeIndex Flow::AddNode(Node& node, Node::Parameter parameter)
	{
		std::size_t index = 0;

		NodeInfos info;
		info.category = NodeCategory::NormalNode;
		info.version = 1;
		info.node = &node;
		info.parameter = std::move(parameter);

		for (index = 0; index < node_infos.size(); ++index)
		{
			auto& ref = node_infos[index];
			if (ref.category == NodeCategory::Empty)
			{
				info.version = ref.version + 1;
				ref = std::move(info);
				return {index, ref.version};
			}
		}

		node_infos.emplace_back(std::move(info));

		return {index, info.version};
	}

	bool Flow::Remove(NodeIndex const& index)
	{
		if (index.index < node_infos.size())
		{
			auto& target = node_infos[index.index];
			if (target.version == index.version)
			{
				auto info = std::move(target);
				target.category = NodeCategory::Empty;

				std::erase_if(node_direct_edges, [=](Graph::GraphEdge const& edge)
					{
						return edge.from == index.index || edge.to == index.index;
					});

				std::erase_if(node_mutex_edges, [=](Graph::GraphEdge const& edge)
					{
						return edge.from == index.index || edge.to == index.index;
					});

				if (info.category == NodeCategory::SubFlowNode)
				{
					encoded_flow.edges.erase(
						encoded_flow.edges.begin() + info.encode_edges.Begin(),
						encoded_flow.edges.begin() + info.encode_edges.End()
					);

					encoded_flow.encode_infos.erase(
						encoded_flow.encode_infos.begin() + info.encode_nodes.Begin(),
						encoded_flow.encode_infos.begin() + info.encode_nodes.End()
					);

					for (auto& ite : node_infos)
					{
						if (ite.category == NodeCategory::SubFlowNode)
						{
							if (ite.encode_nodes.Begin() >= info.encode_nodes.Begin())
							{
								ite.encode_nodes.WholeForward(ite.encode_nodes.Size());
							}
							if (ite.encode_edges.Begin() >= info.encode_edges.Begin())
							{
								ite.encode_edges.WholeForward(ite.encode_edges.Size());
							}
						}
					}
				}
				return true;
			}
		}
		return false;
	}

	bool Flow::AddDirectEdge(NodeIndex from, NodeIndex direct_to)
	{
		if (IsAvailableIndex(from) && IsAvailableIndex(direct_to) && from != direct_to)
		{
			node_direct_edges.emplace_back( from.index, direct_to.index );
			return true;
		}
		return false;
	}

	bool Flow::AddMutexEdge(NodeIndex from, NodeIndex direct_to)
	{
		if (IsAvailableIndex(from) && IsAvailableIndex(direct_to) && from != direct_to)
		{
			node_mutex_edges.emplace_back(from.index, direct_to.index);
			node_mutex_edges.emplace_back(direct_to.index, from.index);
			return true;
		}
		return false;
	}

	bool Flow::RemoveDirectEdge(NodeIndex from, NodeIndex direct_to)
	{
		if (IsAvailableIndex(from) && IsAvailableIndex(direct_to))
		{
			std::erase_if(node_direct_edges, [=](Graph::GraphEdge const& edge)
				{
					return edge.from == from.index && edge.to == direct_to.index;
				});
			return true;
		}
		return false;
	}

	bool Flow::RemoveMutexEdge(NodeIndex from, NodeIndex direct_to)
	{
		if (IsAvailableIndex(from) && IsAvailableIndex(direct_to))
		{
			std::erase_if(node_mutex_edges, [=](Graph::GraphEdge const& edge)
				{
					return (edge.from == from.index && edge.to == direct_to.index)
						|| (edge.from == direct_to.index && edge.to == from.index);
				});
			return true;
		}
		return false;
	}

	bool Flow::IsAvailableIndex(NodeIndex const& index) const
	{
		if(index.index < node_infos.size() && index.version == node_infos[index.index].version)
		{
			return true;
		}
		return false;
	}

	std::optional<std::size_t> Flow::EncodeNodeTo(
		Flow const& target_flow,
		EncodedFlowNodes& output_encoded_flow,
		std::pmr::memory_resource* temporary_resource
	)
	{

		std::size_t total_zero_out_degree_count = 0;

		struct DetectedInfo
		{
			std::size_t original_index = std::numeric_limits<std::size_t>::max();
			std::size_t mapping_index = std::numeric_limits<std::size_t>::max();
			std::size_t in_degree = 0;
			std::size_t encode_in_degree = 0;
			bool is_sub_flow = false;
		};

		std::pmr::vector<DetectedInfo> temporary_node{temporary_resource};
		temporary_node.reserve(target_flow.node_infos.size());
		std::size_t index = 0;
		std::size_t total_available_node_count = 0;

		for (auto& ite : target_flow.node_infos)
		{
			DetectedInfo info;
			if (ite.category != NodeCategory::Empty)
			{
				info.original_index = index;
				info.is_sub_flow = ite.category == NodeCategory::SubFlowNode;
				total_available_node_count += 1;
			}
			temporary_node.emplace_back(info);
			++index;
		}

		for(auto ite : target_flow.node_direct_edges)
		{
			temporary_node[ite.to].in_degree += 1;
		}

		std::pmr::vector<std::size_t> search_stack{ temporary_resource };
		search_stack.reserve(target_flow.node_infos.size());

		for (std::size_t i = 0; i < temporary_node.size(); ++i)
		{
			auto& info = temporary_node[i];
			if (info.original_index != std::numeric_limits<std::size_t>::max() && info.in_degree == 0)
			{
				info.mapping_index = search_stack.size();
				search_stack.push_back(i);
			}
		}

		for (std::size_t search_index = 0; search_index < search_stack.size(); ++search_index)
		{
			auto old_index = search_stack[search_index];

			for (auto& ite : target_flow.node_direct_edges)
			{
				if (ite.from == old_index)
				{
					auto& to_node = temporary_node[ite.to];
					assert(to_node.in_degree > 0);
					to_node.in_degree -= 1;
					if (to_node.in_degree == 0)
					{
						assert(to_node.mapping_index == std::numeric_limits<std::size_t>::max());
						to_node.mapping_index = search_stack.size();
						search_stack.push_back(ite.to);
					}
				}
			}
		}

		if (search_stack.size() != total_available_node_count)
			return {};

		std::size_t index_offset = 0;

		for(auto search_index : search_stack)
		{
			auto& ref = temporary_node[search_index];
			ref.mapping_index += index_offset;
			if(ref.is_sub_flow)
			{
				index_offset += target_flow.node_infos[search_index].encode_nodes.Size() + 1;
			}
		}

		struct EdgesInfo
		{
			std::uint8_t been_direct_to : 1 = false;
			std::uint8_t reach : 1 = false;
			std::uint8_t need_remove : 1 = false;
		};

		std::pmr::vector<EdgesInfo> temporary_edges{ temporary_resource };
		temporary_edges.resize(target_flow.node_infos.size());
		std::size_t old_node_count = output_encoded_flow.encode_infos.size();

		for (auto current_index : search_stack)
		{

			std::size_t edges_count = 0;

			for (auto ite : target_flow.node_direct_edges)
			{
				if (ite.from == current_index)
				{
					auto& ref = temporary_edges[ite.to];
					ref.been_direct_to = true;
					ref.reach = true;
					edges_count += 1;
				}
			}

			std::size_t new_edges_count = output_encoded_flow.edges.size();

			if (edges_count != 0)
			{

				bool modify = true;

				while (modify)
				{
					modify = false;

					for (auto ite : target_flow.node_direct_edges)
					{
						if (ite.from != current_index && ite.to != current_index)
						{
							auto& from_ref = temporary_edges[ite.from];
							auto& to_ref = temporary_edges[ite.to];

							if (from_ref.reach)
							{
								if(!to_ref.reach)
								{
									to_ref.reach = true;
									modify = true;
								}
								to_ref.need_remove = true;
							}
						}
					}
				}
			}

			

			if(temporary_node[current_index].is_sub_flow)
			{
				EncodedFlowNodes::Info encode_node;
				auto& target_flow_node = target_flow.node_infos[current_index];
				encode_node.category = EncodedFlowNodes::Category::SubFlowBegin;
				encode_node.parameter = target_flow_node.parameter;

				std::size_t old_edge_count = output_encoded_flow.edges.size();

				auto sub_node_span = target_flow_node.encode_nodes.Slice(std::span(target_flow.encoded_flow.encode_infos));

				std::size_t sub_node_start_index = output_encoded_flow.encode_infos.size() + 1;

				if (sub_node_span.size() != 0)
				{
					for (auto& sub_node : sub_node_span)
					{
						if (sub_node.in_degree == 0)
						{
							output_encoded_flow.edges.emplace_back(sub_node_start_index);
						}
						++sub_node_start_index;
					}
				}
				else
				{
					output_encoded_flow.edges.emplace_back(sub_node_start_index);
				}
				

				encode_node.direct_edges = { old_edge_count, output_encoded_flow.edges.size() };

				old_edge_count = output_encoded_flow.edges.size();

				for (auto& ite : target_flow.node_mutex_edges)
				{
					if (ite.from == current_index)
					{
						output_encoded_flow.edges.emplace_back(temporary_node[ite.to].mapping_index);
					}
				}

				encode_node.mutex_edges = { old_edge_count, output_encoded_flow.edges.size() };

				old_edge_count = output_encoded_flow.edges.size();

				output_encoded_flow.encode_infos.emplace_back(encode_node);

				sub_node_start_index = output_encoded_flow.encode_infos.size();

				output_encoded_flow.encode_infos.insert(output_encoded_flow.encode_infos.end(), sub_node_span.begin(), sub_node_span.end());

				auto new_added_node_span = std::span(output_encoded_flow.encode_infos).subspan(sub_node_start_index);

				for(auto& ite : new_added_node_span)
				{
					if(ite.in_degree == 0)
					{
						ite.in_degree = 1;
					}
					ite.direct_edges.WholeOffset(old_edge_count);
					ite.mutex_edges.WholeOffset(old_edge_count);
				}

				auto old_edge_span = target_flow_node.encode_edges.Slice(std::span(target_flow.encoded_flow.edges));

				output_encoded_flow.edges.insert(output_encoded_flow.edges.end(), old_edge_span.begin(), old_edge_span.end());

				auto new_added_edge_span = std::span(output_encoded_flow.edges).subspan(old_edge_count);

				encode_node.category = EncodedFlowNodes::Category::SubFlowEnd;
				encode_node.parameter = target_flow_node.parameter;

				for(auto& ite : new_added_edge_span)
				{
					if(ite == std::numeric_limits<std::size_t>::max())
					{
						ite = output_encoded_flow.encode_infos.size();
						encode_node.in_degree += 1;
					}else
					{
						ite += sub_node_start_index;
					}
				}

				old_edge_count = output_encoded_flow.edges.size();

				if (edges_count == 0)
				{
					total_zero_out_degree_count += 1;
					output_encoded_flow.edges.emplace_back(std::numeric_limits<std::size_t>::max());
				}
				else
				{
					for (std::size_t edge_index = 0; edge_index < temporary_edges.size(); ++edge_index)
					{
						auto& edge_ref = temporary_edges[edge_index];
						if (edge_ref.been_direct_to && !edge_ref.need_remove)
						{
							temporary_node[edge_index].encode_in_degree += 1;
							output_encoded_flow.edges.emplace_back(temporary_node[edge_index].mapping_index);
						}
					}
				}

				encode_node.direct_edges = { old_edge_count, output_encoded_flow.edges.size() };
				old_edge_count = output_encoded_flow.edges.size();

				for (auto& ite : target_flow.node_mutex_edges)
				{
					if (ite.from == current_index)
					{
						output_encoded_flow.edges.emplace_back(temporary_node[ite.to].mapping_index);
					}
				}

				encode_node.mutex_edges = { old_edge_count, output_encoded_flow.edges.size() };
				output_encoded_flow.encode_infos.emplace_back(std::move(encode_node));

			}else
			{
				EncodedFlowNodes::Info encode_node;
				auto& target_flow_node = target_flow.node_infos[current_index];
				encode_node.node = target_flow_node.node;
				encode_node.parameter = target_flow_node.parameter;
				encode_node.category = EncodedFlowNodes::Category::NormalNode;
				
				std::size_t old_edge_count = output_encoded_flow.edges.size();
				if(edges_count == 0)
				{
					total_zero_out_degree_count += 1;
					output_encoded_flow.edges.emplace_back(std::numeric_limits<std::size_t>::max());
				}else
				{
					for (std::size_t edge_index = 0; edge_index < temporary_edges.size(); ++edge_index)
					{
						auto& edge_ref = temporary_edges[edge_index];
						if (edge_ref.been_direct_to && !edge_ref.need_remove)
						{
							temporary_node[edge_index].encode_in_degree += 1;
							output_encoded_flow.edges.emplace_back(temporary_node[edge_index].mapping_index);
						}
					}
				}

				encode_node.direct_edges = { old_edge_count, output_encoded_flow.edges.size()};

				old_edge_count = output_encoded_flow.edges.size();

				for(auto& ite : target_flow.node_mutex_edges)
				{
					if(ite.from == current_index)
					{
						output_encoded_flow.edges.emplace_back(temporary_node[ite.to].mapping_index);
					}
				}

				encode_node.mutex_edges = { old_edge_count, output_encoded_flow.edges.size() };

				output_encoded_flow.encode_infos.emplace_back(std::move(encode_node));
			}

			if(edges_count != 0)
			{
				for(auto& ite : temporary_edges)
				{
					ite.reach = false;
					ite.been_direct_to = false;
					ite.need_remove = false;
				}
			}
		}

		for(auto& ite : temporary_node)
		{
			if(ite.mapping_index != std::numeric_limits<std::size_t>::max())
				output_encoded_flow.encode_infos[old_node_count + ite.mapping_index].in_degree = ite.encode_in_degree;
		}

		return total_zero_out_degree_count;
	}


	/*
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

	bool Flow::UpdateProcessNode_AssumedLocked()
	{
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
					ite.under_process = true;
				}
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
			request_task = real_request_task;
			return true;
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
			request_task = real_request_task;
			return false;
		}
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
			std::lock_guard lg(preprocess_mutex);
			TaskFlowExecuteBegin_AssumedLocked(wrapper);
			AcyclicEdgeCheck_AssumedLocked({}, {});
			UpdateProcessNode_AssumedLocked();
			TaskFlowPostUpdateProcessNode_AssumedLocked(wrapper);
			Start(wrapper);
		}else
		{
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
			TaskFlowExecuteEnd_AssumedLocked(wrapper);
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
		return Commited_AssumedLocked(context, std::move(property), std::move(trigger_property), delay);
	}

	bool Flow::Commited_AssumedLocked(Task::Context& context, Task::Property property, Task::TriggerProperty trigger_property, std::optional<TimeT::time_point> delay)
	{
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

	bool Flow::Commited(Task::ContextWrapper& context, Task::Property property, Task::TriggerProperty trigger_property, std::optional<TimeT::time_point> delay)
	{
		std::lock_guard lg(preprocess_mutex);
		return Commited_AssumedLocked(context, std::move(property), std::move(trigger_property), delay);
	}

	bool Flow::Commited_AssumedLocked(Task::ContextWrapper& context, Task::Property property, Task::TriggerProperty trigger_property, std::optional<TimeT::time_point> delay)
	{
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
	*/

}