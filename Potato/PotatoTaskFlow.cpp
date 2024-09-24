module;

#include <cassert>

module PotatoTaskFlow;

namespace Potato::Task
{

	TaskFlow::TaskFlow(std::pmr::memory_resource* task_flow_resource)
			:
		 preprocess_nodes(task_flow_resource)
		, preprocess_mutex_edges(task_flow_resource)
		, preprocess_direct_edges(task_flow_resource)
		, process_nodes(task_flow_resource)
		, process_edges(task_flow_resource)
	{

	}

	TaskFlow::~TaskFlow()
	{
		{
			std::lock_guard lg(preprocess_mutex);
			for(auto& ite : preprocess_nodes)
			{
				ite.node->owner = 0;
				ite.node->index = std::numeric_limits<std::size_t>::max();
			}
			preprocess_nodes.clear();
			preprocess_direct_edges.clear();
			preprocess_mutex_edges.clear();
		}
		
		{
			std::lock_guard lg(process_mutex);
			for(std::size_t index = temporary_node_offset; index < process_nodes.size(); ++index)
			{
				auto& ref = process_nodes[index];
				ref.node->owner = 0;
				ref.node->index = std::numeric_limits<std::size_t>::max();
			}
			process_edges.clear();
			process_nodes.clear();
		}
	}

	bool TaskFlow::AddNode_AssumedLocked(TaskFlowNode& node, TaskFlowNodeProperty property)
	{
		auto owner_id = reinterpret_cast<std::size_t>(this);
		std::size_t old_size = 0;
		if(node.owner.compare_exchange_strong(old_size, owner_id))
		{
			node.index = preprocess_nodes.size();
			preprocess_nodes.emplace_back(&node,
			0,
			preprocess_nodes.size(),
				property
			);
			need_update = true;
			return true;
		}

		return false;
	}

	bool TaskFlow::AddTemporaryNode_AssumedLocked(TaskFlowNode& node, TaskFlowNodeProperty property, bool(*detect_func)(void* append_data, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex), void* append_data)
	{
		auto owner_id = reinterpret_cast<std::size_t>(this);
		std::size_t old_size = 0;
		if(node.owner.compare_exchange_strong(old_size, owner_id))
		{
			auto old_edge = process_edges.size();
			std::size_t cur_index = process_nodes.size();
			node.index = (std::numeric_limits<std::size_t>::max() / 2) + cur_index;
			std::size_t init_in_degree = 0;

			if(detect_func != nullptr)
			{
				std::vector<std::size_t> search_index;
				std::size_t index = 0;
				for(auto& ite : process_nodes)
				{
					if(ite.init_in_degree == 0)
					{
						search_index.emplace_back(index);
					}
					++index;
				}

				while(!search_index.empty())
				{
					auto top = *search_index.rbegin();
					search_index.pop_back();

					auto& ref = process_nodes[top];

					if(ref.status != Status::DONE)
					{
						if((*detect_func)(append_data, *ref.node, ref.property, {top, temporary_node_offset}))
						{
							if(top < temporary_node_offset && ref.status == Status::READY)
							{
								process_edges.push_back(top);
								ref.in_degree += 1;
								continue;
							}else
							{
								ref.need_append_mutex = true;
								append_direct_edge.push_back({top, cur_index});
								init_in_degree += 1;
							}
						}
					}

					if(ref.status != Status::READY)
					{
						auto Misc = ref.direct_edges.Slice(std::span(process_edges));
						search_index.insert(search_index.end(), Misc.begin(), Misc.end());
						if(ref.need_append_mutex)
						{
							for(auto& ite : append_direct_edge)
							{
								if(ite.from == top)
									search_index.push_back(ite.to);
							}
						}
					}
				}
			}

			process_nodes.emplace_back(
				Status::READY,
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

			return true;
		}
		return false;
	}


	bool TaskFlow::Remove_AssumedLocked(TaskFlowNode& node)
	{
		if(node.owner == reinterpret_cast<std::size_t>(this) && node.index < (std::numeric_limits<std::size_t>::max() / 2))
		{
			auto index = node.index;
			auto& remove_ref = preprocess_nodes[index];
			node.index = std::numeric_limits<std::size_t>::max();
			node.owner = 0;
			auto tar = remove_ref.in_degree;
			preprocess_mutex_edges.erase(
				std::remove_if(preprocess_mutex_edges.begin(), preprocess_mutex_edges.end(), [&](PreprocessMutexEdge& edge)
					{
						return (edge.node1 == index || edge.node2 == index);
					}),
				preprocess_mutex_edges.end()
			);

			preprocess_direct_edges.erase(
				std::remove_if(preprocess_direct_edges.begin(), preprocess_direct_edges.end(), [&](PreprocessDirectEdge& edge)
				{
					if(edge.from == index || edge.to == index)
					{
						preprocess_nodes[edge.to].in_degree -= 1;
						return true;
					}else
					{
						if(edge.from > index)
							edge.from -= 1;
						if(edge.to > index)
							edge.to -= 1;
					}
					return false;
				}),
				preprocess_direct_edges.end()
			);
			
			preprocess_nodes.erase(preprocess_nodes.begin() + index);

			for(std::size_t i = 0; i < preprocess_nodes.size(); ++i)
			{
				auto& ref = preprocess_nodes[i];
				if(ref.topology_degree >= tar)
					ref.topology_degree -= 1;
				if(ref.node->index > index)
				{
					ref.node->index = i;
				}
			}
			need_update = true;
			return true;
		}
		return false;
	}


	bool TaskFlow::RemoveDirectEdge_AssumedLocked(TaskFlowNode& from, TaskFlowNode& direct_to)
	{
		if(from.owner == reinterpret_cast<std::size_t>(this) && direct_to.owner == reinterpret_cast<std::size_t>(this))
		{
			auto f = from.index;
			auto t = direct_to.index;
			if(f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;

			for(std::size_t i = 0; i < preprocess_direct_edges.size(); ++i)
			{
				auto cur = preprocess_direct_edges[i];
				if(cur.from == f && cur.to == t)
				{
					preprocess_nodes[cur.to].in_degree -= 1;
					preprocess_direct_edges.erase(preprocess_direct_edges.begin() + i);
					need_update = true;
					return true;
				}
			}
		}
		return false;
	}

	bool TaskFlow::AddMutexEdge_AssumedLocked(TaskFlowNode& from, TaskFlowNode& direct_to)
	{
		if (from.owner == reinterpret_cast<std::size_t>(this) && direct_to.owner == reinterpret_cast<std::size_t>(this))
		{
			auto f = from.index;
			auto t = direct_to.index;
			if (f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;
			preprocess_mutex_edges.emplace_back(f, t);
			need_update = true;
			return true;
		}
		return false;
	}

	bool TaskFlow::AddDirectEdge_AssumedLocked(TaskFlowNode& from, TaskFlowNode& direct_to, std::pmr::memory_resource* temp_resource)
	{
		if (from.owner == reinterpret_cast<std::size_t>(this) && direct_to.owner == reinterpret_cast<std::size_t>(this))
		{
			auto f = from.index;
			auto t = direct_to.index;
			if (f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;

			auto& node_f = preprocess_nodes[f];
			auto& node_t = preprocess_nodes[t];
			auto node_f_topology_degree = node_f.topology_degree;
			auto node_t_topology_degree = node_t.topology_degree;

			if(node_f_topology_degree > node_t_topology_degree)
			{
				preprocess_direct_edges.emplace_back(f, t);
				node_t.in_degree += 1;
				need_update = true;
				return true;
			}else
			{
				std::size_t min_topology_degree_to_from_node = preprocess_nodes.size() + 1;
				std::size_t max_topology_degree_from_to_node = 0;

				for(auto& ite : preprocess_direct_edges)
				{
					if(ite.to == f)
					{
						min_topology_degree_to_from_node = std::min(min_topology_degree_to_from_node, preprocess_nodes[ite.from].topology_degree + 1);
					}

					if (ite.from == t)
					{
						max_topology_degree_from_to_node = std::max(max_topology_degree_from_to_node, preprocess_nodes[ite.to].topology_degree + 1);
					}
				}

				bool move_left = (min_topology_degree_to_from_node > node_t_topology_degree + 1);
				bool move_right = (max_topology_degree_from_to_node < node_f_topology_degree + 1);


				if(move_left && move_right)
				{
					preprocess_direct_edges.emplace_back(f, t);
					node_t.in_degree += 1;
					std::swap(node_f.topology_degree, node_t.topology_degree);

					return true;
				}else if(move_left)
				{
					node_t.in_degree += 1;
					preprocess_direct_edges.emplace_back(f, t);
					for(auto& ite : preprocess_nodes)
					{
						if(ite.topology_degree == node_f_topology_degree)
						{
							ite.topology_degree = node_t_topology_degree;
						}else if(ite.topology_degree > node_f_topology_degree && ite.topology_degree <= node_t_topology_degree)
						{
							ite.topology_degree -= 1;
						}
					}
					need_update = true;
					return true;
				}else if(move_right)
				{
					node_t.in_degree += 1;
					preprocess_direct_edges.emplace_back(f, t);
					for (auto& ite : preprocess_nodes)
					{
						if (ite.topology_degree == node_t_topology_degree)
						{
							ite.topology_degree = node_f_topology_degree;
						}
						else if (ite.topology_degree >= node_f_topology_degree && ite.topology_degree < node_t_topology_degree)
						{
							ite.topology_degree += 1;
						}
					}
					need_update = true;
					return true;
				}else
				{

					struct NodePro
					{
						std::size_t in_degree = 0;
						std::size_t ref_index = 0;
						bool exported = false;
					};

					std::pmr::vector<NodePro> cur_in_degree(temp_resource);
					cur_in_degree.reserve(preprocess_nodes.size());
					std::pmr::vector<std::size_t> mapping_array(temp_resource);
					mapping_array.resize(preprocess_nodes.size(), 0);

					std::size_t total_node = 0;

					std::size_t index = 0;
					for(auto& ite : preprocess_nodes)
					{
						cur_in_degree.emplace_back(ite.in_degree, index++);
					}

					cur_in_degree[t].in_degree += 1;

					bool Finded = true;
					while(Finded)
					{
						Finded = false;
						for(auto& ite : cur_in_degree)
						{
							if(!ite.exported)
							{
								if(ite.in_degree == 0)
								{
									Finded = true;
									ite.exported = true;
									mapping_array[ite.ref_index] = total_node + 1;
									total_node += 1;
									if(ite.ref_index == f)
									{
										cur_in_degree[t].in_degree -= 1;
									}
									for(auto& ite2 : preprocess_direct_edges)
									{
										if(ite2.from == ite.ref_index)
										{
											cur_in_degree[ite2.to].in_degree -= 1;
										}
									}
								}
							}
						}
					}
					if(total_node != cur_in_degree.size())
					{
						return false;
					}

					preprocess_nodes[t].in_degree += 1;

					preprocess_direct_edges.emplace_back(f, t);

					assert(!preprocess_nodes.empty());

					for(std::size_t i = 0; i < preprocess_nodes.size(); ++i)
					{
						preprocess_nodes[i].topology_degree = total_node - mapping_array[i];
					}
					need_update = true;

					return true;
				}
			}
		}
		return false;
	}

	bool TaskFlow::Update_AssumedLocked()
	{
		if(current_status == Status::DONE || current_status == Status::READY)
		{
			current_status = Status::READY;
			finished_task = 0;
			{
				std::lock_guard lg2(preprocess_mutex);
				if(need_update)
				{
					need_update = false;
					process_nodes.clear();
					process_edges.clear();
					for(auto& ite : preprocess_nodes)
					{
						process_nodes.emplace_back(
							Status::READY,
							ite.in_degree,
							0,
							Misc::IndexSpan<>{},
							Misc::IndexSpan<>{},
							ite.in_degree,
							ite.node,
							ite.property,
							0,
							false
						);
						ite.node->Update();
					}
					for(std::size_t i = 0; i < process_nodes.size(); ++i)
					{
						auto& ref = process_nodes[i];
						auto s = process_edges.size();
						for(auto& ite : preprocess_mutex_edges)
						{
							if(ite.node1 == i)
							{
								process_edges.emplace_back(ite.node2);
							}else if(ite.node2 == i)
							{
								process_edges.emplace_back(ite.node1);
							}
						}
						ref.mutex_edges = {s, process_edges.size()};
						s = process_edges.size();
						for(auto& ite : preprocess_direct_edges)
						{
							if(ite.from == i)
							{
								process_edges.emplace_back(ite.to);
							}
						}
						ref.direct_edges = {s, process_edges.size()};
					}
					temporary_node_offset = process_nodes.size();
					temporary_edge_offset = process_edges.size();
					append_direct_edge.clear();
					return true;
				}
			}

			for(std::size_t index = temporary_node_offset; index < process_nodes.size(); ++index)
			{
				auto& ref = process_nodes[index];
				ref.node->owner = 0;
				ref.node->index = std::numeric_limits<std::size_t>::max();
			}

			process_nodes.resize(temporary_node_offset);
			process_edges.resize(temporary_edge_offset);
			for(auto& ite : process_nodes)
			{
				ite.status = Status::READY;
				ite.in_degree = ite.init_in_degree;
				ite.mutex_degree = 0;
				ite.need_append_mutex = false;
				ite.node->Update();
			}
			append_direct_edge.clear();
			
			return true;
		}
		return false;
	}

	bool TaskFlow::Commited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property)
	{
		if(current_status == Status::READY && owner == 0)
		{
			TaskProperty tem_property
			{
				property.display_name,
				{0, 0},
				property.filter
			};
			if(context.CommitTask(this, tem_property))
			{
				current_status = Status::RUNNING;
				return true;
			}
			
		}else if(current_status != Status::DONE)
		{
			assert(false);
		}
		return false;
	}

	bool TaskFlow::SubTaskCommited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property)
	{
		if (current_status == Status::READY)
		{
			TaskProperty tem_property
			{
				property.display_name,
				{0, 0},
				property.filter
			};
			if (context.CommitTask(this, tem_property))
			{
				current_status = Status::RUNNING;
				return true;
			}

		}
		else if (current_status != Status::DONE)
		{
			assert(false);
		}
		return false;
	}

	bool TaskFlow::Commited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property, std::chrono::steady_clock::time_point time_point)
	{
		if(current_status == Status::READY && owner == 0)
		{
			TaskProperty tem_property
			{
				property.display_name,
				{0, 0},
				property.filter
			};
			if(context.CommitDelayTask(this, time_point, tem_property))
			{
				current_status = Status::RUNNING;
				running_property = property;
				return true;
			}
			
		}else if(current_status != Status::DONE)
		{
			assert(false);
		}
		return false;
	}

	void TaskFlow::TaskExecute(TaskContextWrapper& status)
	{
		auto node = reinterpret_cast<TaskFlowNode*>(status.task_property.user_data[0]);
		auto index = status.task_property.user_data[1];
		if(node == nullptr)
		{
			TaskFlowContext context{
				status.context,
				status.status,
				status.task_property.display_name,
				status.task_property.filter,
				status.thread_property,
				{},
				this,
				0
			};
			if(index == 0)
			{
				TaskFlowExecuteBegin(context);

				{
					std::lock_guard lg(process_mutex);
					if(process_nodes.size() == 0)
					{
						status.task_property.user_data[1] = 1;
						status.context.CommitTask(this, status.task_property);
					}else
					{
						for(std::size_t i = 0; i < process_nodes.size(); ++i)
						{
							auto& ite = process_nodes[i];
							TryStartupNode_AssumedLock(status.context, ite, i);
						}
					}
					return;
				}
				
			}else if(index == 1)
			{
				TaskFlow::Ptr parent;
				std::size_t cur_node_identity;
				{
					std::lock_guard lg(process_mutex);
					current_status = Status::DONE;
					parent = std::move(parent_node);
					cur_node_identity = current_identity;
				}
				TaskFlowExecuteEnd(context);
				if(parent)
				{
					parent->ContinuePauseNode(status.context, cur_node_identity);
				}
			}else
			{
				assert(false);
			}
		}else
		{
			TaskFlowContext context{
				status.context,
				status.status,
				status.task_property.display_name,
				status.task_property.filter,
				status.thread_property,
				node,
				this,
				index
			};

			node->TaskFlowNodeExecute(context);
			std::lock_guard lg(process_mutex);
			auto& ref = process_nodes[index];
			FinishNode_AssumedLock(status.context, ref, index);
			return;
		}
	}

	bool TaskFlow::FinishNode_AssumedLock(TaskContext& context, ProcessNode& node, std::size_t index)
	{
		if(node.status == Status::RUNNING)
		{
			assert(node.pause_count == 0);
			node.status = Status::DONE;

			{
				auto temp_span = std::span(process_nodes).subspan(temporary_node_offset);
				std::size_t index = temporary_node_offset;
				for(auto& ite : temp_span)
				{
					TryStartupNode_AssumedLock(context, ite, temporary_node_offset);
					++index;
				}
			}

			if(node.need_append_mutex)
			{
				for(auto& ite : append_direct_edge)
				{
					if(ite.from == index)
					{
						auto& tref = process_nodes[ite.to];
						tref.in_degree -= 1;
						TryStartupNode_AssumedLock(context, tref, ite.to);
					}
				}
			}
			auto mutex_span = node.mutex_edges.Slice(std::span(process_edges));
			for(auto ite : mutex_span)
			{
				auto& tref = process_nodes[ite];
				tref.mutex_degree -= 1;
				TryStartupNode_AssumedLock(context, tref, ite);
			}
			auto edge_span = node.direct_edges.Slice(std::span(process_edges));
			for(auto ite : edge_span)
			{
				auto& tref = process_nodes[ite];
				tref.in_degree -= 1;
				TryStartupNode_AssumedLock(context, tref, ite);
			}
			
			finished_task += 1;
			if(finished_task == process_nodes.size())
			{
				TaskProperty tem_pro{
					running_property.display_name,
					{0, 1},
					running_property.filter
				};
				context.CommitTask(this, tem_pro);
			}
			return true;
		}else if(node.status == Status::RUNNING_NEED_PAUSE)
		{
			node.status = Status::PAUSE;
			return true;
		}
		return false;
	}


	bool TaskFlow::TryStartupNode_AssumedLock(TaskContext& context, ProcessNode& node, std::size_t index)
	{
		if(node.status == Status::READY && node.mutex_degree == 0 && node.in_degree == 0)
		{
			TaskProperty pro{
				node.property.display_name,
				{reinterpret_cast<std::size_t>(node.node.GetPointer()), index},
				node.property.filter
			};
			if(context.CommitTask(this, pro))
			{
				auto mutex_span = node.mutex_edges.Slice(std::span(process_edges));
				for(auto ite : mutex_span)
				{
					process_nodes[ite].mutex_degree += 1;
				}
				node.status = Status::RUNNING;
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::MarkNodePause(std::size_t node_identity)
	{
		std::lock_guard lg(process_mutex);
		if(node_identity < process_nodes.size())
		{
			auto& ref = process_nodes[node_identity];
			switch(ref.status)
			{
			case Status::RUNNING:
				ref.pause_count += 1;
				ref.status = Status::RUNNING_NEED_PAUSE;
				return true;
			case Status::RUNNING_NEED_PAUSE:
			case Status::PAUSE:
				ref.pause_count += 1;
				return true;
			}
		}
		assert(false);
		return false;
	}

	bool TaskFlow::ContinuePauseNode(TaskContext& context, std::size_t node_identity)
	{
		std::lock_guard lg(process_mutex);
		if(node_identity < process_nodes.size())
		{
			auto& ref = process_nodes[node_identity];
			switch(ref.status)
			{
			case Status::RUNNING_NEED_PAUSE:
				assert(ref.pause_count > 0);
				ref.pause_count -= 1;
				if(ref.pause_count == 0)
					ref.status = Status::RUNNING;
				return true;
			case Status::PAUSE:
				assert(ref.pause_count > 0);
				ref.pause_count -= 1;
				if(ref.pause_count == 0)
				{
					ref.status = Status::RUNNING;
					auto re = FinishNode_AssumedLock(context, ref, node_identity);
					assert(re);
					return true;
				}
				return true;
			default:
				break;
			}
		}
		assert(false);
		return false;
	}

	void TaskFlow::TaskFlowNodeExecute(TaskFlowContext& status)
	{
		std::lock_guard lg(process_mutex);
		switch(current_status)
		{
		case Status::READY:
			{
				std::lock_guard lg(parent_mutex);
				assert(!parent_node);
				TaskProperty pro{
					status.node_property.display_name,
					{0, 0},
					status.node_property.filter
				};

				if(SubTaskCommited_AssumedLocked(status.context, status.node_property))
				{
					running_property = status.node_property;
					current_status = Status::RUNNING;
					parent_node = std::move(status.flow);
					current_identity = status.node_identity;
					parent_node->MarkNodePause(status.node_identity);
				}
				break;
			}
		case Status::DONE:
			return;
		default:
			{
				assert(false);
				break;
			}
		}
	}
}