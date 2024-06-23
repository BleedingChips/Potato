module;

#include <cassert>

module PotatoTaskFlow;

namespace Potato::Task
{


	auto TaskFlow::Node::Create(
		TaskFlow* owner, 
		TaskFlowNode::Ptr reference_node,
		UserData::Ptr user_data,
		NodeProperty property, 
		std::size_t index, 
		std::pmr::memory_resource* resource
	)
		-> Ptr
	{
		auto cur_layout = IR::Layout::Get<Node>();
		auto str_name = IR::Layout::GetArray<char8_t>(property.display_name.size());

		auto offset = IR::InsertLayoutCPP(cur_layout, str_name);
		IR::FixLayoutCPP(cur_layout);

		auto re = Potato::IR::MemoryResourceRecord::Allocate(resource, cur_layout);
		if(re)
		{
			auto str = re.GetByte() + offset;
			std::memcpy(str, property.display_name.data(), property.display_name.size());
			return new (re.Get()) Node(re, owner, std::move(reference_node), std::move(user_data),  property.filter, std::u8string_view{reinterpret_cast<char8_t*>(str), property.display_name.size()},  index);
		}
		return {};
	}

	TaskFlow::Node::Node(
		IR::MemoryResourceRecord record, 
		Pointer::ObserverPtr<TaskFlow> owner, 
		TaskFlowNode::Ptr reference_node,
		UserData::Ptr user_data,
		TaskFilter filter, 
		std::u8string_view display_name, 
		std::size_t index
	)
		: record(record), owner(owner), reference_node(std::move(reference_node)), filter(std::move(filter)), reference_id(index),display_name(display_name), user_data(std::move(user_data))
	{
		
	}

	void TaskFlow::Node::Release()
	{
		auto re = record;
		this->~Node();
		re.Deallocate();
	}

	TaskFlow::TaskFlow(std::chrono::steady_clock::duration, std::pmr::memory_resource* task_flow_resource, MemorySetting memory_setting)
		: resources(memory_setting)
		, raw_nodes(task_flow_resource)
		, raw_mutex_edges(task_flow_resource)
		, raw_direct_edges(task_flow_resource)
		, process_nodes(task_flow_resource)
		, process_edges(task_flow_resource)
		, listen_duration(listen_duration)
	{

	}

	auto TaskFlow::AddNode_AssumedLock(TaskFlowNode::Ptr node, NodeProperty property, UserData::Ptr user_data)
		->Node::Ptr
	{
		if(node)
		{
			auto tnode = Node::Create(this, std::move(node), std::move(user_data), std::move(property),  raw_nodes.size(), resources.node_resource);
			if(tnode)
			{
				raw_nodes.emplace_back(
					tnode,
					0,
					raw_nodes.size()
				);
				need_update = true;
				return tnode;
			}
		}
		return {};
	}

	bool TaskFlow::Remove_AssumedLock(Node& node)
	{
		if(node.owner == this)
		{
			if(node.reference_id == std::numeric_limits<std::size_t>::max())
				return false;

			auto index = node.reference_id;
			auto& remove_ref = raw_nodes[index];
			remove_ref.reference_node->reference_id = std::numeric_limits<std::size_t>::max();
			auto tar = remove_ref.in_degree;
			raw_mutex_edges.erase(
				std::remove_if(raw_mutex_edges.begin(), raw_mutex_edges.end(), [&](RawMutexEdge& edge)
					{
						return (edge.node1 == index || edge.node2 == index);
					}),
				raw_mutex_edges.end()
			);

			raw_direct_edges.erase(
				std::remove_if(raw_direct_edges.begin(), raw_direct_edges.end(), [&](RawDirectEdge& edge)
				{
					if(edge.from == index || edge.to == index)
					{
						raw_nodes[edge.to].in_degree -= 1;
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
				raw_direct_edges.end()
			);
			
			raw_nodes.erase(raw_nodes.begin() + index);

			for(std::size_t i = 0; i < raw_nodes.size(); ++i)
			{
				auto& ref = raw_nodes[i];
				if(ref.topology_degree >= tar)
					ref.topology_degree -= 1;
				if(i > index)
				{
					ref.reference_node->reference_id = i;
				}
			}
			need_update = true;
			return true;
		}
		return false;
	}


	bool TaskFlow::RemoveDirectEdge_AssumedLock(Node& from, Node& direct_to)
	{
		if(from.owner == this && direct_to.owner == this)
		{
			auto f = from.reference_id;
			auto t = direct_to.reference_id;
			if(f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;

			for(std::size_t i = 0; i < raw_direct_edges.size(); ++i)
			{
				auto cur = raw_direct_edges[i];
				if(cur.from == f && cur.to == t)
				{
					raw_nodes[cur.to].in_degree -= 1;
					raw_direct_edges.erase(raw_direct_edges.begin() + i);
					need_update = true;
					return true;
				}
			}
		}
		return false;
	}

	bool TaskFlow::AddMutexEdge_AssumedLock(Node& from, Node& direct_to)
	{
		if (from.owner == this && direct_to.owner == this)
		{
			auto f = from.reference_id;
			auto t = direct_to.reference_id;
			if (f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;
			raw_mutex_edges.emplace_back(f, t);
			need_update = true;
			return true;
		}
		return false;
	}

	bool TaskFlow::AddDirectEdge_AssumedLock(Node& from, Node& direct_to, std::pmr::memory_resource* temp_resource)
	{
		if (from.owner == this && direct_to.owner == this)
		{
			auto f = from.reference_id;
			auto t = direct_to.reference_id;
			if (f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;

			auto& node_f = raw_nodes[f];
			auto& node_t = raw_nodes[t];
			auto node_f_topology_degree = node_f.topology_degree;
			auto node_t_topology_degree = node_t.topology_degree;

			if(node_f_topology_degree > node_t_topology_degree)
			{
				raw_direct_edges.emplace_back(f, t);
				node_t.in_degree += 1;
				need_update = true;
				return true;
			}else
			{
				std::size_t min_topology_degree_to_from_node = raw_nodes.size() + 1;
				std::size_t max_topology_degree_from_to_node = 0;

				for(auto& ite : raw_direct_edges)
				{
					if(ite.to == f)
					{
						min_topology_degree_to_from_node = std::min(min_topology_degree_to_from_node, raw_nodes[ite.from].topology_degree + 1);
					}

					if (ite.from == t)
					{
						max_topology_degree_from_to_node = std::max(max_topology_degree_from_to_node, raw_nodes[ite.to].topology_degree + 1);
					}
				}

				bool move_left = (min_topology_degree_to_from_node > t + 1);
				bool move_right = (max_topology_degree_from_to_node < f + 1);


				if(move_left && move_right)
				{

					node_t.in_degree += 1;
					std::swap(node_f.topology_degree, node_t.topology_degree);

					return true;
				}else if(move_left)
				{
					node_t.in_degree += 1;
					raw_direct_edges.emplace_back(f, t);
					for(auto& ite : raw_nodes)
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
					raw_direct_edges.emplace_back(f, t);
					for (auto& ite : raw_nodes)
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
					cur_in_degree.reserve(raw_nodes.size());
					std::pmr::vector<std::size_t> mapping_array(temp_resource);
					mapping_array.resize(raw_nodes.size(), 0);

					std::size_t total_node = 0;

					std::size_t index = 0;
					for(auto& ite : raw_nodes)
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
									for(auto& ite2 : raw_direct_edges)
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

					raw_nodes[t].in_degree += 1;

					raw_direct_edges.emplace_back(f, t);

					assert(!raw_nodes.empty());

					for(std::size_t i = 0; i < raw_nodes.size(); ++i)
					{
						raw_nodes[i].topology_degree = total_node - mapping_array[i];
					}
					need_update = true;

					return true;
				}
			}
		}
		return false;
	}

	bool TaskFlow::Update_AssumedLock()
	{
		if(current_status == Status::DONE || current_status == Status::READY)
		{
			current_status = Status::READY;
			finished_task = 0;
			{
				std::lock_guard lg2(raw_mutex);
				if(need_update)
				{
					need_update = false;
					process_nodes.clear();
					process_edges.clear();
					for(auto& ite : raw_nodes)
					{
						process_nodes.emplace_back(
							Status::READY,
							ite.in_degree,
							0,
							Misc::IndexSpan<>{},
							Misc::IndexSpan<>{},
							ite.in_degree,
							ite.reference_node
						);
						ite.reference_node->reference_node->Update();
					}
					for(std::size_t i = 0; i < process_nodes.size(); ++i)
					{
						auto& ref = process_nodes[i];
						auto s = process_edges.size();
						for(auto& ite : raw_mutex_edges)
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
						for(auto& ite : raw_direct_edges)
						{
							if(ite.from == i)
							{
								process_edges.emplace_back(ite.to);
							}
						}
						ref.direct_edges = {s, process_edges.size()};
					}
					return true;
				}
			}
			for(auto& ite : process_nodes)
			{
				ite.status = Status::READY;
				ite.in_degree = ite.init_in_degree;
				ite.mutex_degree = 0;
				ite.reference_node->reference_node->Update();
			}
			return true;
		}
		return false;
	}

	bool TaskFlow::Commited_AssumedLock(TaskContext& context, NodeProperty property)
	{
		if(current_status == Status::READY)
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
				running_property = property;
				return true;
			}
			
		}else if(current_status != Status::DONE)
		{
			assert(false);
		}
		return false;
	}

	bool TaskFlow::Commited_AssumedLock(TaskContext& context, NodeProperty property, std::chrono::steady_clock::time_point time_point)
	{
		if(current_status == Status::READY)
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

	void TaskFlow::TaskExecute(ExecuteStatus& status)
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
							TryStartupNode(status.context, ite, i);
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
			finished_task += 1;
			if(finished_task == process_nodes.size())
			{
				TaskProperty tem_pro{
					running_property.display_name,
					{0, 1},
					running_property.filter
				};
				status.context.CommitTask(this, tem_pro);
				return;
			}
			auto& ref = process_nodes[index];
			FinishNode_AssumedLock(status.context, ref);
			return;
		}
	}

	bool TaskFlow::FinishNode_AssumedLock(TaskContext& context, ProcessNode& node)
	{
		if(node.status == Status::RUNNING)
		{
			node.status = Status::DONE;
			auto mutex_span = node.mutex_edges.Slice(std::span(process_edges));
			for(auto ite : mutex_span)
			{
				auto& tref = process_nodes[ite];
				tref.mutex_degree -= 1;
				TryStartupNode(context, tref, ite);
			}
			auto edge_span = node.direct_edges.Slice(std::span(process_edges));
			for(auto ite : edge_span)
			{
				auto& tref = process_nodes[ite];
				tref.in_degree -= 1;
				TryStartupNode(context, tref, ite);
			}
			return true;
		}
		return false;
	}


	bool TaskFlow::TryStartupNode(TaskContext& context, ProcessNode& node, std::size_t index)
	{
		if(node.status == Status::READY && node.mutex_degree == 0 && node.in_degree == 0)
		{
			TaskProperty pro{
				node.reference_node->display_name,
				{reinterpret_cast<std::size_t>(node.reference_node->reference_node.GetPointer()), index},
				node.reference_node->filter
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
			if(ref.status == Status::RUNNING)
			{
				ref.status = Status::PAUSE;
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
			if(ref.status == Status::PAUSE)
			{
				ref.status = Status::RUNNING;
				auto re = FinishNode_AssumedLock(context, ref);
				assert(re);
				return true;
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

				if(status.context.CommitTask(this, pro))
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