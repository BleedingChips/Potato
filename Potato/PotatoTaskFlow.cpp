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

	Flow::NodeIndex Flow::AddFlowAsNode(Flow const& flow, Node::Ptr sub_flow_node, Node::Parameter parameter, std::pmr::memory_resource* temporary_resource)
	{
		auto old_node_size = encoded_flow.encode_infos.size();
		auto old_edge_size = encoded_flow.edges.size();

		if (EncodeNodeTo(flow, encoded_flow, temporary_resource))
		{
			std::size_t index = 0;

			NodeInfos info;
			info.node = std::move(sub_flow_node);
			info.category = NodeCategory::SubFlowNode;
			info.version = 1;
			info.encode_nodes = { old_node_size, encoded_flow.encode_infos.size() };
			info.encode_edges = { old_edge_size, encoded_flow.edges.size() };
			info.parameter = parameter;

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
		EncodedFlow& output_encoded_flow,
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
			std::uint8_t been_mutex_to : 1 = false;
		};

		std::pmr::vector<EdgesInfo> temporary_edges{ temporary_resource };
		temporary_edges.resize(target_flow.node_infos.size());
		std::size_t old_node_count = output_encoded_flow.encode_infos.size();

		for (auto current_index : search_stack)
		{

			std::size_t edges_count = 0;
			std::size_t mutex_edges_count = 0;

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

			for (auto ite : target_flow.node_mutex_edges)
			{
				if (ite.from == current_index)
				{
					auto& ref = temporary_edges[ite.to];
					ref.been_mutex_to = true;
					mutex_edges_count += 1;
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
				EncodedFlow::Info encode_node;
				auto& target_flow_node = target_flow.node_infos[current_index];
				encode_node.category = EncodedFlow::Category::SubFlowBegin;
				encode_node.parameter = target_flow_node.parameter;
				encode_node.node = target_flow_node.node;
				

				std::size_t old_edge_count = output_encoded_flow.edges.size();

				auto sub_node_span = target_flow_node.encode_nodes.Slice(std::span(target_flow.encoded_flow.encode_infos));

				Misc::IndexSpan<> owning_sub_flow = { output_encoded_flow.encode_infos.size(), output_encoded_flow.encode_infos.size() + sub_node_span.size() + 2};
				encode_node.owning_sub_flow = owning_sub_flow;

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

				if (mutex_edges_count != 0)
				{
					for (std::size_t temp_edge_index = 0; temp_edge_index < temporary_edges.size(); ++temp_edge_index)
					{
						if (temporary_edges[temp_edge_index].been_mutex_to)
						{
							output_encoded_flow.edges.emplace_back(temporary_node[temp_edge_index].mapping_index);
						}
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

					if (ite.owning_sub_flow.Size() == 0)
					{
						ite.owning_sub_flow = owning_sub_flow;
					}else
					{
						ite.owning_sub_flow.WholeOffset(sub_node_start_index);
					}

					ite.direct_edges.WholeOffset(old_edge_count);
					ite.mutex_edges.WholeOffset(old_edge_count);
				}

				auto old_edge_span = target_flow_node.encode_edges.Slice(std::span(target_flow.encoded_flow.edges));

				output_encoded_flow.edges.insert(output_encoded_flow.edges.end(), old_edge_span.begin(), old_edge_span.end());

				auto new_added_edge_span = std::span(output_encoded_flow.edges).subspan(old_edge_count);

				encode_node.category = EncodedFlow::Category::SubFlowEnd;
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

				if (new_added_node_span.empty())
				{
					encode_node.in_degree += 1;
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

				if (mutex_edges_count != 0)
				{
					for (std::size_t temp_edge_index = 0; temp_edge_index < temporary_edges.size(); ++temp_edge_index)
					{
						if (temporary_edges[temp_edge_index].been_mutex_to)
						{
							output_encoded_flow.edges.emplace_back(temporary_node[temp_edge_index].mapping_index);
						}
					}
				}

				encode_node.mutex_edges = { old_edge_count, output_encoded_flow.edges.size() };
				output_encoded_flow.encode_infos.emplace_back(std::move(encode_node));

			}else
			{
				EncodedFlow::Info encode_node;
				auto& target_flow_node = target_flow.node_infos[current_index];
				encode_node.node = target_flow_node.node;
				encode_node.parameter = target_flow_node.parameter;
				encode_node.category = EncodedFlow::Category::NormalNode;
				
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

				if (mutex_edges_count != 0)
				{
					for (std::size_t temp_edge_index = 0; temp_edge_index < temporary_edges.size(); ++temp_edge_index)
					{
						if (temporary_edges[temp_edge_index].been_mutex_to)
						{
							output_encoded_flow.edges.emplace_back(temporary_node[temp_edge_index].mapping_index);
						}
					}
				}

				encode_node.mutex_edges = { old_edge_count, output_encoded_flow.edges.size() };

				output_encoded_flow.encode_infos.emplace_back(std::move(encode_node));
			}

			if(edges_count != 0 || mutex_edges_count != 0)
			{
				for(auto& ite : temporary_edges)
				{
					ite.reach = false;
					ite.been_direct_to = false;
					ite.need_remove = false;
					ite.been_mutex_to = false;
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


	Executor::Executor(std::pmr::memory_resource* resource)
		: encoded_flow(resource), encoded_flow_execute_state(resource), template_node(resource), template_edges(resource)
	{
		
	}

	bool Executor::UpdateFromFlow(Flow const& target_flow, std::pmr::memory_resource* temporary_resource)
	{
		std::lock_guard lg(execute_state_mutex);
		return UpdateFromFlow_AssumedLocked(target_flow, temporary_resource);
	}

	bool Executor::UpdateFromFlow_AssumedLocked(Flow const& target_flow, std::pmr::memory_resource* temporary_resource)
	{
		if (execute_state == ExecuteState::State::Running)
		{
			return false;
		}

		std::lock_guard lg2(encoded_flow_mutex);
		auto old_flow_node_count = encoded_flow.encode_infos.size();
		auto old_flow_edge_count = encoded_flow.edges.size();
		auto encode_result = Flow::EncodeNodeTo(target_flow, encoded_flow, temporary_resource);
		if (encode_result.has_value())
		{
			encoded_flow_out_degree = *encode_result;
			encoded_flow.encode_infos.erase(encoded_flow.encode_infos.begin(), encoded_flow.encode_infos.begin() + old_flow_node_count);
			encoded_flow.edges.erase(encoded_flow.edges.begin(), encoded_flow.edges.begin() + old_flow_edge_count);
			
			if (old_flow_edge_count != 0)
			{
				for (auto& ite : encoded_flow.encode_infos)
				{
					ite.direct_edges.WholeForward(old_flow_edge_count);
					ite.mutex_edges.WholeForward(old_flow_edge_count);
				}
			}

			if (old_flow_node_count != 0)
			{
				for (auto& ite : encoded_flow.edges)
				{
					if (ite != std::numeric_limits<std::size_t>::max())
						ite -= old_flow_node_count;
				}
			}

			execute_state = ExecuteState::State::Ready;

			execute_out_degree = encoded_flow_out_degree;
			encoded_flow_execute_state.clear();

			for (auto& ite : encoded_flow.encode_infos)
			{
				ExecuteState state;
				state.in_degree = ite.in_degree;
				encoded_flow_execute_state.emplace_back(state);
			}

			encoded_flow_node_count_for_execute = encoded_flow.encode_infos.size();
			template_edges.clear();
			current_template_node_count = 0;

			std::lock_guard lg3(template_node_mutex);
			template_node.clear();

			return true;
		}
		return false;
	}

	bool Executor::UpdateState()
	{
		std::lock_guard lg(execute_state_mutex);
		return UpdateState_AssumedLocked();
	}

	void Executor::TaskExecute(Task::Context& context, Parameter& parameter)
	{
		auto index = parameter.custom_data.data1;

		if (index == std::numeric_limits<std::size_t>::max() - 1)
		{
			Node::Parameter current_parameter = parameter;
			{
				std::lock_guard lg(execute_state_mutex);
				assert(execute_state == ExecuteState::State::WaitingBegin);
				execute_state = ExecuteState::State::Running;
				current_parameter.custom_data = executor_parameter.custom_data;
			}
			BeginFlow(context, current_parameter);
			std::shared_lock sl(encoded_flow_mutex);
			for (std::size_t index = 0; index < encoded_flow_execute_state.size(); ++index)
			{
				TryStartupNode_AssumedLocked(context, index);
			}
			if (execute_out_degree == 0)
			{
				std::lock_guard lg(execute_state_mutex);
				assert(execute_state == ExecuteState::State::Running);
				execute_state = ExecuteState::State::WaitingEnd;
				index = std::numeric_limits<std::size_t>::max();
			}else
			{
				return;
			}
		}

		if (index == std::numeric_limits<std::size_t>::max())
		{
			Node::Parameter exe_parameter = parameter;
			{
				std::lock_guard lg(execute_state_mutex);
				assert(execute_state == ExecuteState::State::WaitingEnd);
				exe_parameter.custom_data = executor_parameter.custom_data;
			}
			EndFlow(context, exe_parameter);
			{
				std::lock_guard lg(execute_state_mutex);
				FinishFlow_AssumedLocked(context, exe_parameter);
			}
			
			return;
		}

		TaskFlow::Node::Ptr node;
		TaskFlow::Node::Parameter current_node_parameter;
		EncodedFlow::Category category = EncodedFlow::Category::NormalNode;

		{

			if (index < std::numeric_limits<std::size_t>::max() / 2)
			{
				std::shared_lock sl(encoded_flow_mutex);
				assert(index < encoded_flow.encode_infos.size());
				auto& ref = encoded_flow.encode_infos[index];
				node = ref.node;
				current_node_parameter = ref.parameter;
				category = ref.category;
			}else
			{
				std::shared_lock sl(template_node_mutex);
				auto tem_index = index - std::numeric_limits<std::size_t>::max() / 2;
				auto& ref = template_node[tem_index];
				node = ref.node;
				current_node_parameter = ref.parameter;
			}
		}

		Controller controller{ *this, current_node_parameter, index };
		controller.category = category;

		if (node)
		{
			ExecuteNode(context, *node, controller);
		}

		{
			std::lock_guard lg(execute_state_mutex);
			if (index >= std::numeric_limits<std::size_t>::max() / 2)
			{
				index = index - std::numeric_limits<std::size_t>::max() / 2 + encoded_flow_node_count_for_execute;
			}
			FinishNode_AssumedLocked(context, index);
		}
	}

	void Executor::ExecuteNode(Task::Context& context, TaskFlow::Node& target_node, Controller& controller)
	{
		target_node.TaskFlowNodeExecute(context, controller);
	}

	void Executor::FinishNode_AssumedLocked(Task::Context& context, std::size_t index)
	{
		auto& ref = encoded_flow_execute_state[index];

		assert(ref.state == ExecuteState::State::Running || ref.state == ExecuteState::State::Pause);

		if (ref.pause_count != 0)
		{
			ref.state = ExecuteState::State::Pause;
		}else
		{
			ref.state = ExecuteState::State::Done;

			if (index < encoded_flow_node_count_for_execute)
			{
				std::shared_lock sl(encoded_flow_mutex);
				auto& encode_node = encoded_flow.encode_infos[index];
				auto dir_edges = encode_node.direct_edges.Slice(std::span(encoded_flow.edges));
				auto mut_edges = encode_node.mutex_edges.Slice(std::span(encoded_flow.edges));

				for (auto ite : dir_edges)
				{
					if (ite != std::numeric_limits<std::size_t>::max())
					{
						encoded_flow_execute_state[ite].in_degree -= 1;
						TryStartupNode_AssumedLocked(context, ite);
					}
					else
					{
						execute_out_degree -= 1;
					}
				}

				if (encode_node.category != EncodedFlow::Category::SubFlowBegin)
				{
					for (auto ite : mut_edges)
					{
						encoded_flow_execute_state[ite].mutex_degree -= 1;
						TryStartupNode_AssumedLocked(context, ite);
					}
				}
			}

			if (ref.has_template_edges)
			{
				for (auto& edge : template_edges)
				{
					if (edge.from == index)
					{
						assert(encoded_flow_execute_state[edge.to].in_degree > 0);
						encoded_flow_execute_state[edge.to].in_degree -= 1;
						TryStartupNode_AssumedLocked(context, edge.to);
					}
				}
			}

			if (index >= encoded_flow_node_count_for_execute)
			{
				execute_out_degree -= 1;
				current_template_node_count -= 1;
				if (current_template_node_count == 0)
				{
					template_edges.clear();

					encoded_flow_execute_state.resize(encoded_flow_node_count_for_execute);
					for (auto& ite : encoded_flow_execute_state)
					{
						ite.has_template_edges = false;
					}

					std::lock_guard lg(template_node_mutex);
					template_node.clear();
				}
			}

			if (execute_out_degree == 0)
			{
				execute_state = ExecuteState::State::WaitingEnd;
				auto end_parameter = executor_parameter;
				end_parameter.custom_data.data1 = std::numeric_limits<std::size_t>::max();
				end_parameter.custom_data.data2 = 0;
				context.Commit(*this, end_parameter);
			}
		}
	}

	bool Executor::Commit_AssumedLocked(Task::Context& context, Task::Node::Parameter flow_parameter)
	{
		if (execute_state == ExecuteState::State::Ready)
		{
			execute_state = ExecuteState::State::WaitingBegin;
			executor_parameter = flow_parameter;
			flow_parameter.custom_data.data1 = std::numeric_limits<std::size_t>::max() - 1;
			if (context.Commit(*this, flow_parameter))
			{
				return true;
			}
			execute_state = ExecuteState::State::Ready;
		}
		return false;
	}

	bool Executor::UpdateState_AssumedLocked()
	{
		if (execute_state == ExecuteState::State::Done)
		{
			execute_state = ExecuteState::State::Ready;
			std::shared_lock sl(encoded_flow_mutex);
			assert(encoded_flow_execute_state.size() >= encoded_flow.encode_infos.size());
			encoded_flow_execute_state.resize(encoded_flow.encode_infos.size());
			for (std::size_t i = 0; i < encoded_flow.encode_infos.size(); ++i)
			{
				auto& tar = encoded_flow_execute_state[i];
				tar.in_degree = encoded_flow.encode_infos[i].in_degree;
				tar.mutex_degree = 0;
				tar.pause_count = 0;
				tar.state = ExecuteState::State::Ready;
				tar.has_template_edges = false;
			}
			template_edges.clear();
			current_template_node_count = 0;
			execute_out_degree = encoded_flow_out_degree;

			std::lock_guard lg3(template_node_mutex);
			template_node.clear();
			return true;
		}
		return false;
	}
	
	void Executor::FinishFlow_AssumedLocked(Task::Context& context, Task::Node::Parameter parameter)
	{
		execute_state = ExecuteState::State::Done;
	}

	void Executor::TryStartupNode_AssumedLocked(Task::Context& context, std::size_t index)
	{
		auto& state = encoded_flow_execute_state[index];
		if (state.in_degree == 0 && state.mutex_degree == 0 && state.state == ExecuteState::State::Ready)
		{
			Task::Node::Parameter node_parameter;

			if (index < encoded_flow_node_count_for_execute)
			{
				std::shared_lock sl(encoded_flow_mutex);
				auto& encoded_node = encoded_flow.encode_infos[index];
				if (encoded_node.category != EncodedFlow::Category::SubFlowEnd)
				{
					auto span_mutex_edge = encoded_node.mutex_edges.Slice(std::span(encoded_flow.edges));
					for (auto edge : span_mutex_edge)
					{
						encoded_flow_execute_state[edge].mutex_degree += 1;
					}
				}
				node_parameter.acceptable_mask = encoded_node.parameter.acceptable_mask;
				node_parameter.node_name = encoded_node.parameter.node_name;
			}
			else {
				std::shared_lock sl(template_node_mutex);
				auto& temp_node = template_node[index - encoded_flow_node_count_for_execute];
				node_parameter.acceptable_mask = temp_node.parameter.acceptable_mask;
				node_parameter.node_name = temp_node.parameter.node_name;
			}

			if (index >= encoded_flow_node_count_for_execute)
			{
				index = index - encoded_flow_node_count_for_execute + std::numeric_limits<std::size_t>::max() / 2;
			}

			node_parameter.custom_data.data1 = index;
			node_parameter.custom_data.data2 = 0;
			auto node = context.Commit(*this, node_parameter);
			assert(node);
			state.state = ExecuteState::State::Running;
		}
	}

	void Executor::AddTaskNodeRef() const { AddTaskFlowExecutorRef(); }
	void Executor::SubTaskNodeRef() const { SubTaskFlowExecutorRef(); }

	Executor::PauseMountPoint Executor::CreatePauseMountPoint(std::size_t encoded_flow_index)
	{
		std::lock_guard lg(execute_state_mutex);
		if (encoded_flow_index < encoded_flow_execute_state.size())
		{
			auto& ref = encoded_flow_execute_state[encoded_flow_index];
			if (ref.state == ExecuteState::State::Running || ref.state == ExecuteState::State::Pause)
			{
				ref.pause_count += 1;
				ref.state = ExecuteState::State::Pause;
				return {this, encoded_flow_index };
			}
		}
		return {};
	}

	Executor::PauseMountPoint::PauseMountPoint(PauseMountPoint&& point)
		: exectutor(std::move(point.exectutor)), encoded_flow_index(point.encoded_flow_index)
	{
	}

	Executor::PauseMountPoint::~PauseMountPoint()
	{
		assert(!exectutor);
		if(exectutor)
		{
			exectutor->TerminalPauseMountPoint(encoded_flow_index);
			exectutor.Reset();
		}
	}

	bool Executor::PauseMountPoint::Continue(Task::Context& context)
	{
		if (exectutor)
		{
			exectutor->ContinuePauseMountPoint(context, encoded_flow_index);
			exectutor.Reset();
			return true;
		}
		return false;
	}


	bool Executor::ContinuePauseMountPoint(Task::Context& context, std::size_t encoded_flow_index)
	{
		std::lock_guard lg(execute_state_mutex);
		if (encoded_flow_index < encoded_flow_execute_state.size())
		{
			auto& ref = encoded_flow_execute_state[encoded_flow_index];
			assert(ref.pause_count > 0);
			assert(ref.state == ExecuteState::State::Running || ref.state == ExecuteState::State::Pause);
			ref.pause_count -= 1;

			if (ref.pause_count == 0)
			{
				std::shared_lock sl(encoded_flow_mutex);
				FinishNode_AssumedLocked(context, encoded_flow_index);
				return true;
			}
		}
		return false;
	}

	bool Executor::TerminalPauseMountPoint(std::size_t encoded_flow_index)
	{
		std::lock_guard lg(execute_state_mutex);
		if (encoded_flow_index < encoded_flow_execute_state.size())
		{
			auto& ref = encoded_flow_execute_state[encoded_flow_index];
			assert(ref.pause_count > 0);
			assert(ref.state == ExecuteState::State::Running || ref.state == ExecuteState::State::Pause);
			ref.pause_count -= 1;
			if (ref.pause_count == 0)
			{
				ref.state = ExecuteState::State::PauseTerminal;
			}
			return true;
		}
		return false;
	}

	void Executor::TaskTerminal(Parameter& parameter) noexcept
	{
		auto index = parameter.custom_data.data1;
		std::lock_guard lg(execute_state_mutex);
		if (index == std::numeric_limits<std::size_t>::max())
		{
			execute_state = ExecuteState::State::FlowTerminal;
		}else if (index == std::numeric_limits<std::size_t>::max() - 1)
		{
			return;
		}else
		{
			if (index >= std::numeric_limits<std::size_t>::max() / 2)
			{
				index = index - std::numeric_limits<std::size_t>::max() / 2;
			}
			execute_state = ExecuteState::State::FlowTerminal;
			assert(encoded_flow_execute_state.size() > index);
			encoded_flow_execute_state[index].state = ExecuteState::State::FlowTerminal;
		}
	}

	bool Executor::AddTemporaryNode(Task::Context& context, TaskFlow::Node& target_node, TaskFlow::Node::Parameter parameter, bool(*func)(void* data, Sequencer& sequencer), void* append_data, std::pmr::memory_resource* resource)
	{
		std::lock_guard lg(execute_state_mutex);
		std::shared_lock sl1(encoded_flow_mutex);

		if(execute_state == ExecuteState::State::Running)
		{

			TemplateNode t_node;
			t_node.node = &target_node;
			t_node.parameter = parameter;

			{
				std::lock_guard sl2(template_node_mutex);
				template_node.emplace_back(std::move(t_node));
			}
			++current_template_node_count;
			

			std::size_t template_in_degree = 0;
			bool template_need_template = false;

			struct TemplateSearch
			{
				ExecuteState::State state = ExecuteState::State::Ready;
				std::uint8_t reached : 1 = false;
				std::uint8_t need_mutex : 1 = false;
				std::size_t in_degree = 0;
				std::size_t depth = 0;
			};

			std::pmr::vector<TemplateSearch> template_search{ resource };

			template_search.resize(encoded_flow_execute_state.size());

			for (std::size_t index = 0; index < encoded_flow_execute_state.size(); ++index)
			{
				auto const& ref = encoded_flow_execute_state[index];
				auto& tar = template_search[index];
				if (ref.state != ExecuteState::State::Ready && ref.state != ExecuteState::State::Running && ref.state != ExecuteState::State::Pause)
				{
					tar.reached = true;
				}else
				{
					tar.in_degree = ref.in_degree;
				}
				tar.state = ref.state;
			}

			bool modify = true;
			while (modify)
			{
				modify = false;

				for (std::size_t index = 0; index < template_search.size(); ++index)
				{
					auto& tar = template_search[index];
					if (!tar.reached && tar.in_degree == 0)
					{
						tar.reached = true;

						if (func != nullptr)
						{
							Sequencer sequencer{*this, index, tar.depth };
							tar.need_mutex = func(append_data, sequencer);
						}

						assert(index < encoded_flow_execute_state.size());
						bool has_template_edge = encoded_flow_execute_state[index].has_template_edges;

						if (index < encoded_flow_node_count_for_execute)
						{
							assert(index < encoded_flow.encode_infos.size());
							auto& ref = encoded_flow.encode_infos[index];
							auto dir = ref.direct_edges.Slice(std::span(encoded_flow.edges));

							if (tar.need_mutex && tar.state == ExecuteState::State::Ready)
							{
								for (auto ite : dir)
								{
									if (ite != std::numeric_limits<std::size_t>::max())
									{
										assert(ite < template_search.size());
										auto& search = template_search[ite];
										search.reached = true;
									}
								}
							}
							else {
								for (auto ite : dir)
								{
									if (ite != std::numeric_limits<std::size_t>::max())
									{
										assert(ite < template_search.size());
										auto& search = template_search[ite];
										assert(search.in_degree > 0);
										search.in_degree -= 1;
										if (search.in_degree == 0)
										{
											search.depth = tar.depth + 1;
										}
									}
								}
							}
						}

						if (has_template_edge)
						{
							if (
								tar.need_mutex 
								&& tar.state == ExecuteState::State::Ready 
								&& index < encoded_flow_node_count_for_execute
								)
							{
								for (auto ite : template_edges)
								{
									if (ite.from == index)
									{
										assert(ite.to < template_search.size());
										auto& search = template_search[ite.to];
										search.reached = true;
									}
								}
							}
							else {
								for (auto ite : template_edges)
								{
									if (ite.from == index)
									{
										assert(ite.to < template_search.size());
										auto& search = template_search[ite.to];
										assert(search.in_degree > 0);
										search.in_degree -= 1;
										if (search.in_degree == 0 && ite.to < index)
										{
											modify = true;
										}
									}
								}
							}
						}
					}
				}
			}
	
			bool has_director = false;
			for (std::size_t index = 0; index < template_search.size(); ++index)
			{
				auto& ref = template_search[index];
				if (ref.need_mutex)
				{
					auto& tar = encoded_flow_execute_state[index];

					if (tar.state == ExecuteState::State::Ready && index < encoded_flow_node_count_for_execute)
					{
						template_edges.emplace_back(encoded_flow_execute_state.size(), index);
						tar.in_degree += 1;
						template_need_template = true;
					}
					else {
						tar.has_template_edges = true;
						template_edges.emplace_back(index, encoded_flow_execute_state.size());
						++template_in_degree;
						has_director = true;
					}
				}
			}

			ExecuteState state;
			state.has_template_edges = template_need_template;
			state.in_degree = template_in_degree;

			encoded_flow_execute_state.emplace_back(state);
			++execute_out_degree;

			if (!has_director)
			{
				TryStartupNode_AssumedLocked(context, encoded_flow_execute_state.size() - 1);
			}
			return true;
		}
		return false;
	}


	Executor::PauseMountPoint Controller::MarkCurrentAsPause()
	{
		return executor.CreatePauseMountPoint(encoded_flow_index);
	}

	std::optional<Node::Parameter> Sequencer::GetParameter(std::size_t index) const
	{
		if (index < executor.encoded_flow_node_count_for_execute)
		{
			std::shared_lock sl(executor.encoded_flow_mutex);
			return executor.encoded_flow.encode_infos[index].parameter;
		}
		else {
			std::shared_lock sl(executor.template_node_mutex);
			assert(index - executor.encoded_flow_node_count_for_execute < executor.template_node.size());
			return executor.template_node[index - executor.encoded_flow_node_count_for_execute].parameter;
		}
	}
	Misc::IndexSpan<> Sequencer::GetSubFlow(std::size_t index) const
	{
		if (index < executor.encoded_flow_node_count_for_execute)
		{
			std::shared_lock sl(executor.encoded_flow_mutex);
			return executor.encoded_flow.encode_infos[index].owning_sub_flow;
		}
		else {
			return {};
		}
	}

	struct DefaultFlow : public Executor, public IR::MemoryResourceRecordIntrusiveInterface
	{
		DefaultFlow(IR::MemoryResourceRecord record)
			: Executor(record.GetMemoryResource()), MemoryResourceRecordIntrusiveInterface(record)
		{
			
		}
		virtual void AddTaskFlowExecutorRef() const { MemoryResourceRecordIntrusiveInterface::AddRef(); }
		virtual void SubTaskFlowExecutorRef() const { MemoryResourceRecordIntrusiveInterface::SubRef(); }
	};

	Executor::Ptr Executor::Create(std::pmr::memory_resource* resource)
	{
		auto re = IR::MemoryResourceRecord::Allocate<DefaultFlow>(resource);
		if (re)
		{
			return new(re.Get()) DefaultFlow{ re };
		}
		return {};
	}

	bool Executor::Commit(Task::Context& context, Task::Node::Parameter flow_parameter)
	{
		std::lock_guard lg(execute_state_mutex);
		return Commit_AssumedLocked(context, flow_parameter);
	}

}