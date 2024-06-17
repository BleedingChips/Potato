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

	TaskFlow::TaskFlow(std::pmr::memory_resource* task_flow_resource, MemorySetting memory_setting)
		: resources(memory_setting)
		, raw_nodes(task_flow_resource)
		, raw_edges(task_flow_resource)
		, process_nodes(task_flow_resource)
		, process_edges(task_flow_resource)
	{

	}

	auto TaskFlow::AddNode(TaskFlowNode::Ptr node, NodeProperty property, UserData::Ptr user_data)
		->Node::Ptr
	{
		if(node)
		{
			std::lock_guard lg(raw_mutex);
			auto tnode = Node::Create(this, std::move(node), std::move(user_data), std::move(property),  raw_nodes.size(), resources.node_resource);
			if(tnode)
			{
				raw_nodes.emplace_back(
					tnode,
					0
				);
				need_update = true;
				return tnode;
			}
		}
		return {};
	}

	bool TaskFlow::Remove(Node& node)
	{
		if(node.owner == this)
		{
			std::lock_guard lg(raw_mutex);
			if(node.reference_id == std::numeric_limits<std::size_t>::max())
				return false;

			auto index = node.reference_id;
			raw_edges.erase(
				std::remove_if(raw_edges.begin(), raw_edges.end(), [&](RawEdge& edge)
				{
					if(edge.from == index || edge.to == index)
					{
						if(edge.type == EdgeType::Direct && edge.from == index)
						{
							raw_nodes[edge.to].in_degree -= 1;
						}
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
				raw_edges.end()
			);
			raw_nodes[index].reference_node->reference_id = std::numeric_limits<std::size_t>::max();
			raw_nodes.erase(raw_nodes.begin() + index);
			for(std::size_t i = index; i < raw_nodes.size(); ++i)
			{
				raw_nodes[i].reference_node->reference_id = i;
			}
			need_update = true;
			return true;
		}
		return false;
	}


	bool TaskFlow::RemoveDirectEdge(Node& form, Node& direct_to)
	{
		if(form.owner == this && direct_to.owner == this)
		{
			std::lock_guard lg(raw_mutex);
			auto f = form.reference_id;
			auto t = direct_to.reference_id;
			if(f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;

			for(std::size_t i = 0; i < raw_edges.size(); ++i)
			{
				auto cur = raw_edges[i];
				if(cur.type == EdgeType::Direct && cur.from == f && cur.to == t)
				{
					raw_nodes[cur.to].in_degree -= 1;
					raw_edges.erase(raw_edges.begin() + i);
					need_update = true;
					return true;
				}
			}
		}
		return false;
	}

	bool TaskFlow::AddMutexEdge(Node& form, Node& direct_to)
	{
		if (form.owner == this && direct_to.owner == this)
		{
			std::lock_guard lg(raw_mutex);
			auto f = form.reference_id;
			auto t = direct_to.reference_id;
			if (f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;
			raw_edges.emplace_back(EdgeType::Mutex, f, t);
			raw_edges.emplace_back(EdgeType::Mutex, t, f);
			need_update = true;
			return true;
		}
		return false;
	}

	bool TaskFlow::AddDirectEdge(Node& form, Node& direct_to, std::pmr::memory_resource* temp_resource)
	{
		if (form.owner == this && direct_to.owner == this)
		{
			std::lock_guard lg(raw_mutex);
			auto f = form.reference_id;
			auto t = direct_to.reference_id;
			if (f == t || f == std::numeric_limits<std::size_t>::max() || t == std::numeric_limits<std::size_t>::max())
				return false;
			if(f > t)
			{
				raw_edges.emplace_back(EdgeType::Direct, f, t);
				raw_nodes[t].in_degree += 1;
				need_update = true;
				return true;
			}else
			{
				std::optional<std::size_t> min_to_from_node;
				std::optional<std::size_t> max_from_to_node;

				for(auto& ite : raw_edges)
				{
					if(ite.type == EdgeType::Direct)
					{
						if(ite.to == f)
						{
							min_to_from_node = 
								min_to_from_node.has_value() ?
								std::min(*min_to_from_node, ite.from)
								:  ite.from;
						}

						if(ite.from == t)
						{
							max_from_to_node = 
								max_from_to_node.has_value() ?
								std::max(*max_from_to_node, ite.to)
								:  ite.to;
						}
					}
				}

				bool move_left = (!min_to_from_node.has_value() || *min_to_from_node > t);
				bool move_right = (!max_from_to_node.has_value() || *max_from_to_node < f);


				if(move_left && move_right)
				{
					auto& fn = raw_nodes[f];
					auto& tn = raw_nodes[t];

					tn.in_degree += 1;

					raw_edges.emplace_back(
						EdgeType::Direct,
						f, t
					);

					std::swap(raw_nodes[f], raw_nodes[t]);

					fn.reference_node->reference_id = f;
					tn.reference_node->reference_id = t;

					auto Fix = [=](std::size_t& ref)
					{
						if(ref == f)
							ref = t;
						else if(ref == t)
							ref = f;
					};

					for(auto& ite : raw_edges)
					{
						Fix(ite.from);
						Fix(ite.to);
					}

					return true;
				}else if(move_left)
				{
					raw_nodes[t].in_degree += 1;
					raw_edges.emplace_back(
						EdgeType::Direct,
						f, t
					);
					auto fn = std::move(raw_nodes[f]);
					raw_nodes.erase(raw_nodes.begin() + f);
					raw_nodes.insert(raw_nodes.begin() + t, std::move(fn));

					for(std::size_t i = f; i < t + 1; ++i)
					{
						raw_nodes[i].reference_node->reference_id = i;
					}

					auto Fix = [=](std::size_t& ref)
					{
						if(ref == f)
							ref = t;
						else if(ref > f && ref <= t)
						{
							ref -= 1;
						}
					};

					for(auto& ite : raw_edges)
					{
						Fix(ite.from);
						Fix(ite.to);
					}

					need_update = true;
					return true;
				}else if(move_right)
				{
					auto tn = std::move(raw_nodes[t]);
					tn.in_degree += 1;
					raw_edges.emplace_back(
						EdgeType::Direct,
						f, t
					);
					raw_nodes.erase(raw_nodes.begin() + t);
					raw_nodes.insert(raw_nodes.begin() + f, std::move(tn));

					for(std::size_t i = f; i < t + 1; ++i)
					{
						raw_nodes[i].reference_node->reference_id = i;
					}

					auto Fix = [=](std::size_t& ref)
					{
						if(ref == t)
							ref = f;
						else if(ref >= f && ref < t)
						{
							ref += 1;
						}
					};

					for(auto& ite : raw_edges)
					{
						Fix(ite.from);
						Fix(ite.to);
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
									for(auto& ite2 : raw_edges)
									{
										if(ite2.type == EdgeType::Direct && ite2.from == ite.ref_index)
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

					for(auto& ite : mapping_array)
						ite = total_node - ite;

					raw_edges.emplace_back(EdgeType::Direct, f, t);

					for(auto& ite : raw_edges)
					{
						ite.from = mapping_array[ite.from];
						ite.to = mapping_array[ite.to];
					}

					assert(!raw_nodes.empty());

					for(std::size_t i = 0; i < mapping_array.size(); ++i)
					{
						auto tar = mapping_array[i];
						while(i != tar)
						{
							std::swap(mapping_array[i], mapping_array[tar]);
							std::swap(raw_nodes[i], raw_nodes[tar]);
							tar = mapping_array[i];
						}
						raw_nodes[i].reference_node->reference_id = i;
					}
					need_update = true;

					return true;
				}
			}
		}
		return false;
	}

	bool TaskFlow::Update()
	{
		std::lock_guard lg(process_mutex);
		if(current_status == Status::DONE || current_status == Status::READY)
		{
			current_status = Status::READY;
			bool need_process_update = true;
			finished_task = 0;
			{
				std::lock_guard lg2(raw_mutex);
				if(need_update)
				{
					need_process_update = false;
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
							ite.reference_node
						);
						ite.reference_node->reference_node->Update();
					}
					for(std::size_t i = 0; i < process_nodes.size(); ++i)
					{
						auto& ref = process_nodes[i];
						auto s = process_edges.size();
						for(auto& ite : raw_edges)
						{
							if(ite.type == EdgeType::Mutex && ite.from == i)
							{
								process_edges.emplace_back(ite.to);
							}
						}
						ref.mutex_edges = {s, process_edges.size()};
						s = process_edges.size();
						for(auto& ite : raw_edges)
						{
							if(ite.type == EdgeType::Direct && ite.from == i)
							{
								process_edges.emplace_back(ite.to);
							}
						}
						ref.direct_edges = {s, process_edges.size()};
					}
				}
			}
			if(need_process_update)
			{
				for(auto& ite : process_nodes)
				{
					ite.status = Status::READY;
					ite.in_degree = ite.init_in_degree;
					ite.mutex_degree = 0;
					ite.reference_node->reference_node->Update();
				}
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::Commited(TaskContext& context, NodeProperty property)
	{
		std::lock_guard lg(process_mutex);
		if(current_status == Status::READY)
		{
			this->property = property;
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
				this,
				{},
				0
			};
			if(index == 0)
			{
				TaskFlowExecuteBegin(context);

				{
					std::lock_guard lg(process_mutex);
					current_status = Status::DONE;
					if(process_nodes.size() == 0)
					{
						status.task_property.user_data[1] = 1;
						status.context.CommitTask(this, status.task_property);
						return;
					}else
					{
						for(std::size_t i = 0; i < process_nodes.size(); ++i)
						{
							auto& ite = process_nodes[i];
							TryStartupNode(status.context, ite, i);
						}
					}
				}
				
			}else if(index == 1)
			{
				{
					std::lock_guard lg(process_mutex);
					current_status = Status::DONE;
				}
				TaskFlowExecuteEnd(context);
				std::lock_guard lg(pause_mutex);
				if(pause_owner)
				{
					if(pause_owner->process_nodes.size() > pause_index)
					{
						auto& ref = pause_owner->process_nodes[pause_index];
						if(ref.status == Status::PAUSE)
						{
							ref.status = Status::DONE;
							auto mutex_span = ref.mutex_edges.Slice(std::span(pause_owner->process_edges));
							for(auto ite : mutex_span)
							{
								auto& tref = pause_owner->process_nodes[ite];
								tref.mutex_degree -= 1;
								pause_owner->TryStartupNode(status.context, tref, ite);
							}
							auto edge_span = ref.direct_edges.Slice(std::span(pause_owner->process_edges));
							for(auto ite : edge_span)
							{
								auto& tref = pause_owner->process_nodes[ite];
								tref.in_degree -= 1;
								pause_owner->TryStartupNode(status.context, tref, ite);
							}
						}
					}
				}
			}else
			{
				assert(false);
			}
			return;
		}else
		{
			TaskFlowContext context{
				status.context,
				status.status,
				status.task_property.display_name,
				status.task_property.filter,
				status.thread_property,
				this,
				node,
				index
			};

			node->TaskFlowNodeExecute(context);
			std::lock_guard lg(process_mutex);
			finished_task += 1;
			if(finished_task == process_nodes.size())
			{
				TaskProperty tem_pro{
					property.display_name,
					{0, 1},
					property.filter
				};
				status.context.CommitTask(this, tem_pro);
				return;
			}
			auto& ref = process_nodes[index];
			ref.status = Status::DONE;
			auto mutex_span = ref.mutex_edges.Slice(std::span(process_edges));
			for(auto ite : mutex_span)
			{
				auto& tref = process_nodes[ite];
				tref.mutex_degree -= 1;
				TryStartupNode(status.context, tref, ite);
			}
			auto edge_span = ref.direct_edges.Slice(std::span(process_edges));
			for(auto ite : edge_span)
			{
				auto& tref = process_nodes[ite];
				tref.in_degree -= 1;
				TryStartupNode(status.context, tref, ite);
			}
			return;
		}
	}

	bool TaskFlow::TryStartupNode(TaskContext& context, ProcessNode& node, std::size_t index)
	{
		if(node.status == Status::READY && node.mutex_degree == 0 && node.in_degree == 0)
		{
			TaskProperty pro{
				node.reference_node->display_name,
				{node.reference_node->reference_node, index},
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

	void TaskFlow::TaskFlowNodeExecute(TaskFlowContext& status)
	{
		std::lock_guard lg(pause_mutex);
		if(!pause_owner)
		{
			if(Commited(status.context, {status.display_name, status.filter}))
			{
				pause_owner = std::move(status.flow_node);
				pause_index = status.reference_index;
				std::lock_guard lg(pause_owner->process_mutex);
				pause_owner->process_nodes[status.reference_index].status = Status::PAUSE;
			}
		}
	}

}