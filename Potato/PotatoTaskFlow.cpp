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


	FlowExecutorNode::FlowExecutorNode(std::pmr::memory_resource* resource)
		: encode_flow_nodes(resource), execute_state(resource)
	{
		
	}

	bool FlowExecutorNode::UpdateFromFlow(Flow const& target_flow, std::pmr::memory_resource* temporary_resource)
	{
		std::lock_guard lg(running_state_mutex);

		if (running_state == ExecuteState::State::Running)
		{
			return false;
		}

		std::lock_guard lg2(encode_nodes_mutex);
		auto old_flow_node_count = encode_flow_nodes.encode_infos.size();
		auto old_flow_edge_count = encode_flow_nodes.edges.size();
		auto encode_result = Flow::EncodeNodeTo(target_flow, encode_flow_nodes, temporary_resource);
		if (encode_result.has_value())
		{
			end_indegree = *encode_result;
			encode_flow_nodes.encode_infos.erase(encode_flow_nodes.encode_infos.begin(), encode_flow_nodes.encode_infos.begin() + old_flow_node_count);
			encode_flow_nodes.edges.erase(encode_flow_nodes.edges.begin(), encode_flow_nodes.edges.begin() + old_flow_edge_count);
			running_state = ExecuteState::State::Ready;

			running_end_indegree = end_indegree;
			execute_state.clear();

			for (auto& ite : encode_flow_nodes.encode_infos)
			{
				ExecuteState state;
				state.in_degree = ite.in_degree;
				execute_state.emplace_back(state);
			}

			running_state = ExecuteState::State::Ready;
			return true;
		}
		return false;
	}


	bool FlowExecutorNode::ForceUpdateState()
	{
		std::lock_guard lg(running_state_mutex);
		if (running_state == ExecuteState::State::Done)
		{
			running_state = ExecuteState::State::Ready;
			std::shared_lock sl(encode_nodes_mutex);
			assert(execute_state.size() == encode_flow_nodes.encode_infos.size());
			for (std::size_t i = 0; i < encode_flow_nodes.encode_infos.size(); ++i)
			{
				auto& tar = execute_state[i];
				tar.in_degree = encode_flow_nodes.encode_infos[i].in_degree;
				tar.mutex_degree = 0;
				tar.pause_count = 0;
				tar.state = ExecuteState::State::Ready;
			}
			return true;
		}
		return false;
	}

	void FlowExecutorNode::TaskExecute(Task::Context& context, Parameter& parameter)
	{
		auto index = parameter.custom_data.data1;

		if (index == std::numeric_limits<std::size_t>::max() - 1)
		{
			BeginFlow(context, flow_parameter);
			std::shared_lock sl(encode_nodes_mutex);
			for (std::size_t index = 0; index < execute_state.size(); ++index)
			{
				if (execute_state[index].in_degree == 0)
				{
					TryStartupNode(context, index);
				}
			}
			if (running_end_indegree == 0)
			{
				index = std::numeric_limits<std::size_t>::max();
			}else
			{
				return;
			}
		}

		if (index == std::numeric_limits<std::size_t>::max())
		{
			{
				std::lock_guard lg(running_state_mutex);
				running_state = ExecuteState::State::Done;
			}
			EndFlow(context, flow_parameter);
			return;
		}

		TaskFlow::Node::Ptr node;
		TaskFlow::Node::Parameter current_node_parameter;
		{
			std::shared_lock sl(encode_nodes_mutex);
			auto& ref = encode_flow_nodes.encode_infos[index];
			node = ref.node;
			current_node_parameter = ref.parameter;
		}

		FlowExecutorController controller;

		if (node)
		{
			node->TaskFlowNodeExecute(context, controller, current_node_parameter);
		}

		{
			std::lock_guard lg(running_state_mutex);
			std::shared_lock sl(encode_nodes_mutex);
			execute_state[index].state = ExecuteState::State::Done;
			auto& encode_node = encode_flow_nodes.encode_infos[index];
			auto dir_edges = encode_node.direct_edges.Slice(std::span(encode_flow_nodes.edges));
			auto mut_edges = encode_node.direct_edges.Slice(std::span(encode_flow_nodes.edges));

			for (auto ite : dir_edges)
			{
				if (ite != std::numeric_limits<std::size_t>::max())
				{
					execute_state[ite].in_degree -= 1;
					TryStartupNode(context, ite);
				}
				else
				{
					end_indegree -= 1;
				}
			}

			if (encode_node.category != EncodedFlowNodes::Category::SubFlowBegin)
			{
				for (auto ite : mut_edges)
				{
					execute_state[ite].mutex_degree -= 1;
					TryStartupNode(context, ite);
				}
			}

			if (end_indegree == 0)
			{
				auto end_parameter = flow_parameter;
				flow_parameter.custom_data.data1 = std::numeric_limits<std::size_t>::max();
				context.Commit(*this, end_parameter);
			}
		}
	}

	void FlowExecutorNode::TryStartupNode(Task::Context& context, std::size_t index)
	{
		auto& state = execute_state[index];
		if (state.in_degree == 0 && state.mutex_degree == 0 && state.state == ExecuteState::State::Ready)
		{
			auto& encode_node = encode_flow_nodes.encode_infos[index];
			if (encode_node.category != EncodedFlowNodes::Category::SubFlowEnd)
			{
				auto span_mutex_edge = encode_node.mutex_edges.Slice(std::span(encode_flow_nodes.edges));
				for (auto edge : span_mutex_edge)
				{
					execute_state[edge].mutex_degree += 1;
				}
			}
			Task::Node::Parameter node_parameter;
			node_parameter.custom_data.data1 = index;
			node_parameter.acceptable_mask = encode_node.parameter.acceptable_mask;
			node_parameter.node_name = encode_node.parameter.node_name;
			auto node = context.Commit(*this, node_parameter);
			assert(node);
			state.state = ExecuteState::State::Running;
		}
	}

	void FlowExecutorNode::AddTaskNodeRef() const { AddTaskFlowExecutorRef(); }
	void FlowExecutorNode::SubTaskNodeRef() const { SubTaskFlowExecutorRef(); }

	struct DefaultFlow : public FlowExecutorNode, public IR::MemoryResourceRecordIntrusiveInterface
	{
		DefaultFlow(IR::MemoryResourceRecord record)
			: FlowExecutorNode(record.GetMemoryResource()), MemoryResourceRecordIntrusiveInterface(record)
		{
			
		}
		virtual void AddTaskFlowExecutorRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubTaskFlowExecutorRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }
	};

	FlowExecutorNode::Ptr FlowExecutorNode::Create(std::pmr::memory_resource* resource)
	{
		auto re = IR::MemoryResourceRecord::Allocate<DefaultFlow>(resource);
		if (re)
		{
			return new(re.Get()) DefaultFlow{ re };
		}
		return {};
	}

}