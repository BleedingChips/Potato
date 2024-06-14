module;

#include <cassert>

module PotatoTaskFlow;

namespace Potato::Task
{


	auto TaskFlow::Node::Create(TaskFlow* owner, TaskFlowNode::Ptr reference_node,
				NodeProperty property, std::size_t index, std::pmr::memory_resource* resource)
		-> Ptr
	{
		auto re = Potato::IR::MemoryResourceRecord::Allocate<Node>(resource);
		if(re)
		{
			return new (re.Get()) Node(re, owner, std::move(reference_node), property, index);
		}
		return {};
	}

	TaskFlow::Node::Node(IR::MemoryResourceRecord record, Pointer::ObserverPtr<TaskFlow> owner, TaskFlowNode::Ptr reference_node,
				NodeProperty property, std::size_t index)
		: record(record), owner(owner), reference_node(std::move(reference_node)), property(std::move(property)), reference_id(index)
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

	auto TaskFlow::AddNode(TaskFlowNode::Ptr node, NodeProperty property)
		->Node::Ptr
	{
		if(node)
		{
			std::lock_guard lg(raw_mutex);
			auto tnode = Node::Create(this, std::move(node), std::move(property), raw_nodes.size(), resources.node_resource);
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

	void TaskFlow::TaskFlowNodeExecute(TaskFlowContext& status)
	{
		
	}




	//TaskFlowNodeProcessor::Ptr TaskFlow::CreateProcessor();

	/*
	

	std::optional<std::size_t> TaskFlow::LocatePreCompliedNode(TaskFlowNode& node, std::lock_guard<std::mutex> const& lg) const
	{
		std::size_t i = 0;
		for(auto ite : pre_complied_nodes)
		{
			if(&node == ite.node.GetPointer())
				return i;
			++i;
		}
		return std::nullopt;
	}

	bool TaskFlow::AddNode(TaskFlowNode::Ptr node, NodeProperty property)
	{
		if (node)
		{
			std::lock_guard lg1(pre_compiled_mutex);
			auto li = LocatePreCompliedNode(*node, lg1);
			if(!li.has_value())
			{
				pre_complied_nodes.emplace_back(
					std::move(node),
					property
				);
				need_update = true;
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::Remove(TaskFlowNode::Ptr node)
	{
		if(node)
		{
			std::lock_guard lg1(pre_compiled_mutex);
			auto li = LocatePreCompliedNode(*node, lg1);
			if(li.has_value())
			{
				
				pre_complied_edges.erase(
					std::ranges::remove_if(pre_complied_edges, [&](PreCompiledEdge& edge)->bool
						{
							if(edge.from == *li || edge.to == *li)
							{
								return true;
							}else
							{
								if(edge.from > *li)
									edge.from -= 1;
								if(edge.to > *li)
									edge.to -= 1;
								return false;
							}
						}).begin(),
						pre_complied_edges.end()
						);
				pre_complied_nodes.erase(pre_complied_nodes.begin() + *li);
				need_update = true;
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::AddDirectEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to)
	{
		if(form && direct_to)
		{
			std::lock_guard lg(pre_compiled_mutex);
			auto l1 = LocatePreCompliedNode(*form, lg);
			auto l2 = LocatePreCompliedNode(*direct_to, lg);
			if(l1 && l2 && *l1 != *l2)
			{
				pre_complied_edges.emplace_back(
					EdgeType::Direct, *l1, *l2
				);
				need_update = true;
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::AddMutexEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to)
	{
		if(form && direct_to)
		{
			std::lock_guard lg(pre_compiled_mutex);
			auto l1 = LocatePreCompliedNode(*form, lg);
			auto l2 = LocatePreCompliedNode(*direct_to, lg);
			if (l1 && l2 && *l1 != *l2)
			{
				pre_complied_edges.emplace_back(
					EdgeType::Mutex, *l1, *l2
				);
				pre_complied_edges.emplace_back(
					EdgeType::Mutex, *l2, *l1
				);
				need_update = true;
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::CloneCompliedNodes(TaskNodeExecuteNodes& nodes, TaskFilter ref_filter)
	{
		std::lock_guard lg(compiled_mutex);
		nodes.edges = complied_edges;
		nodes.nodes.clear();
		for(auto& ite : complied_nodes)
		{
			TaskProperty property;
			if(ite.pre_node.property.filter.has_value())
			{
				property.filter = *ite.pre_node.property.filter;
			}else
			{
				property.filter = ref_filter;
			}
			property.display_name = ite.pre_node.property.display_name;
			auto ref_ptr = ite.pre_node.node->Clone();
			if(ref_ptr)
			{
				nodes.nodes.emplace_back(
					std::move(ref_ptr),
					property,
					ite.in_degree,
					ite.in_degree,
					0,
					ite.mutex_edges,
					ite.direct_edges,
					TaskNodeExecuteNodes::RunningState::Idle
				);
			}else
			{
				nodes.edges.clear();
				nodes.nodes.clear();
				return false;
			}
		}
		return true;
	}


	bool TaskFlow::TryUpdate(std::pmr::vector<ErrorNode>* error_output, std::pmr::memory_resource* temp)
	{

		std::lock_guard lg(pre_compiled_mutex);
		if(need_update)
		{
			std::ranges::sort(pre_complied_edges, [](PreCompiledEdge const& l1, PreCompiledEdge const& l2)
			{
				auto so = l1.from <=> l2.from;
				if(so == std::strong_ordering::less)
				{
					return true;
				}else if(so == std::strong_ordering::greater)
				{
					return false;
				}else
				{
					return false;
				}
			});

			struct UpdateNode
			{
				std::size_t in_degree = 0;
				std::size_t temp_degree = 0;
				std::size_t touch_count = 0;
				std::optional<std::size_t> compiled_node_mapping;
				Misc::IndexSpan<> pre_compile_edge;
			};

			std::pmr::vector<UpdateNode> update_nodes(temp);

			update_nodes.resize(pre_complied_nodes.size());

			Misc::IndexSpan<> total_span = {0, pre_complied_edges.size()};

			std::size_t cur_index = 0;
			for(auto& ite : update_nodes)
			{
				auto last_span = total_span.Slice(std::span(pre_complied_edges));
				auto find = std::ranges::find_if(last_span, [=](PreCompiledEdge const& edge) { return edge.from != cur_index; });
				auto offset = std::distance(last_span.begin(), find);
				ite.pre_compile_edge = total_span.SubIndex(0, offset);
				total_span = total_span.SubIndex(offset);
				cur_index++;
			}

			for(auto& ite : pre_complied_edges)
			{
				if(ite.type == EdgeType::Direct)
				{
					update_nodes[ite.to].in_degree += 1;
				}
			}

			std::pmr::vector<std::size_t> temp_complied_edges(temp);
			std::pmr::vector<CompiledNode> temp_complied_nodes(temp);
			std::pmr::vector<std::size_t> search_stack(temp);
			while(temp_complied_nodes.size() < update_nodes.size())
			{
				std::size_t startup_index = 0;
				bool Find = false;
				for(auto& ite : update_nodes)
				{
					if(!ite.compiled_node_mapping.has_value() && ite.in_degree == 0)
					{
						break;
					}else
					{
						++startup_index;
					}
				}

				for(auto& ite : update_nodes)
				{
					ite.touch_count = 0;
				}

				if(startup_index < update_nodes.size())
				{
					search_stack.clear();
					auto& ref = update_nodes[startup_index];
					ref.compiled_node_mapping = temp_complied_nodes.size();
					auto edges = ref.pre_compile_edge.Slice(std::span(pre_complied_edges));
					auto old_size = temp_complied_edges.size();

					for (auto& ite : edges)
					{
						assert(ite.from == startup_index);
						if (ite.type == EdgeType::Mutex)
						{
							temp_complied_edges.push_back(ite.to);
						}
					}

					auto old_size2 = temp_complied_edges.size();

					for (auto& ite : edges)
					{
						assert(ite.from == startup_index);
						if (ite.type == EdgeType::Direct)
						{
							auto& ref = update_nodes[ite.to];
							if(ref.touch_count == 0)
							{
								temp_complied_edges.push_back(ite.to);
								search_stack.emplace_back(ite.to);
								++ref.touch_count;
							}
							ref.in_degree -= 1;
						}
					}

					while(!search_stack.empty())
					{
						auto top = *search_stack.rbegin();
						search_stack.pop_back();
						auto& tem_ref = update_nodes[top];
						if(tem_ref.touch_count == 1)
						{
							auto span_edge = tem_ref.pre_compile_edge.Slice(std::span(pre_complied_edges));
							for (auto& ite : span_edge)
							{
								assert(ite.from == top);
								if (ite.type == EdgeType::Direct)
								{
									update_nodes[ite.to].touch_count += 1;
									search_stack.push_back(ite.to);
								}
							}
						}else if(tem_ref.touch_count >= 2)
						{
							temp_complied_edges.erase(
								std::remove_if(temp_complied_edges.begin() + old_size2, temp_complied_edges.end(), [=](std::size_t i)
									{
										return i == top;
									}),
								temp_complied_edges.end()
							);
						}
					}

					CompiledNode new_one;
					new_one.pre_node = pre_complied_nodes[startup_index];
					new_one.mutex_edges = { old_size, old_size2 };
					new_one.direct_edges = { old_size2, temp_complied_edges.size()};
					temp_complied_nodes.emplace_back(std::move(new_one));
				}else
				{
					if(error_output != nullptr)
					{
						std::size_t index = 0;
						for(auto& ite : update_nodes)
						{
							if (!ite.compiled_node_mapping.has_value())
							{
								error_output->emplace_back(
									pre_complied_nodes[index].node,
									pre_complied_nodes[index].property.display_name
									);
							}
							++index;
						}
					}
					return false;
				}
			}

			for(auto& ite : temp_complied_edges)
			{
				assert(update_nodes[ite].compiled_node_mapping);
				ite = *update_nodes[ite].compiled_node_mapping;
			}

			for(auto& ite : temp_complied_nodes)
			{
				auto span = ite.direct_edges.Slice(std::span(temp_complied_edges));
				for(auto& ite2 : span)
				{
					temp_complied_nodes[ite2].in_degree += 1;
				}
			}

			std::lock_guard lg2(compiled_mutex);
			complied_nodes = temp_complied_nodes;
			complied_edges = temp_complied_edges;
			need_update = false;
			return true;
		}
		return false;
	}

	TaskFlowExecute::Ptr TaskFlow::Commit(TaskContext& context, TaskProperty property, std::pmr::memory_resource* resource)
	{
		if(resource != nullptr)
		{
			auto re = Potato::IR::MemoryResourceRecord::Allocate<IndependenceTaskFlowExecute>(resource);
			if(re)
			{
				auto ptr = new (re.Get()) IndependenceTaskFlowExecute{this, re, property};
				ptr->Commit(context, {});
				return ptr;
			}
		}
		return {};
	}

	IndependenceTaskFlowExecute::IndependenceTaskFlowExecute(TaskFlow::Ptr owner, Potato::IR::MemoryResourceRecord record, TaskProperty property)
		: owner(std::move(owner)), record(record), property(property), nodes(record.GetMemoryResource())
	{
		if(this->owner)
		{
			this->owner->CloneCompliedNodes(nodes, property.filter);
		}
	}

	void IndependenceTaskFlowExecute::Release()
	{
		auto re = record;
		this->~IndependenceTaskFlowExecute();
		record.Deallocate();
	}

	bool IndependenceTaskFlowExecute::Commit(TaskContext& context, std::optional<std::chrono::steady_clock::time_point> delay_point)
	{
		if(owner)
		{
			std::lock_guard lg(mutex);
			if(state == RunningState::Idle)
			{
				TaskProperty cur_pro = property;
				cur_pro.user_data = { 0, 0 };
				if(!delay_point.has_value())
				{
					if (context.CommitTask(this, cur_pro))
					{
						state = RunningState::Running;
						return true;
					}
				}else
				{
					if (context.CommitDelayTask(this, *delay_point, cur_pro))
					{
						state = RunningState::Running;
						return true;
					}
				}
			}
		}
		return false;
	}

	void IndependenceTaskFlowExecute::TaskExecute(ExecuteStatus& status)
	{
		auto ptr = reinterpret_cast<TaskFlowNode*>(status.task_property.user_data[0]);
		if (ptr != nullptr)
		{
			TaskFlowStatus tf_status{
				status.context,
				status.task_property,
				status.thread_property,
				*this,
				*owner,
				status.task_property.user_data[1],
			};
			ptr->TaskFlowNodeExecute(tf_status);
			std::lock_guard lg(mutex);
			FinishTaskFlowNode(status.context, status.task_property.user_data[1]);
			if (run_task != nodes.nodes.size())
			{
				return;
			}else
			{
				TaskProperty pro = property;
				pro.user_data = {0, 1};
				status.context.CommitTask(this, pro);
			}
		}
		else
		{
			if(status.task_property.user_data.datas[1] == 0)
			{
				if(owner)
				{
					owner->TaskFlowExecuteBegin(status, *this);
				}

				std::lock_guard lg(mutex);
				StartupTaskFlow(status.context);
				
				if (run_task != nodes.nodes.size())
				{
					return;
				}
				else
				{
					TaskProperty pro = property;
					pro.user_data = {0, 1};
					status.context.CommitTask(this, pro);
				}
			}else
			{
				{
					std::lock_guard lg(mutex);
					state = RunningState::Done;
				}
				if (owner)
				{
					owner->TaskFlowExecuteEnd(status, *this);
				}
			}
		}
	}

	std::size_t IndependenceTaskFlowExecute::StartupTaskFlow(TaskContext& context)
	{
		//std::lock_guard lg(mutex);
		std::size_t count = 0;
		for (std::size_t i = 0; i < nodes.nodes.size(); ++i)
		{
			if (TryStartSingleTaskFlowImp(context, i))
			{
				count += 1;
			}
		}
		return count;
	}

	bool IndependenceTaskFlowExecute::TryStartSingleTaskFlowImp(TaskContext& context, std::size_t fast_index)
	{
		assert(fast_index < nodes.nodes.size());
		auto& ref = nodes.nodes[fast_index];
		assert(ref.node);
		if (ref.current_in_degree == 0 && ref.mutex_count == 0 && ref.status == RunningState::Idle)
		{
			auto pro = ref.property;
			pro.user_data = { reinterpret_cast<std::size_t>(ref.node.GetPointer()), fast_index };

			if (context.CommitTask(
				this,
				pro
			))
			{
				auto mutex_span = ref.mutex_span.Slice(std::span(nodes.edges));
				for (auto ite : mutex_span)
				{
					nodes.nodes[ite].mutex_count += 1;
				}
				ref.status = RunningState::Running;
				return true;
			}
		}
		return false;
	}

	std::size_t IndependenceTaskFlowExecute::FinishTaskFlowNode(TaskContext& context, std::size_t fast_index)
	{
		std::size_t count = 0;
		//std::lock_guard lg(mutex);
		run_task += 1;
		assert(fast_index < nodes.nodes.size());
		auto& ref = nodes.nodes[fast_index];

		auto mutex_span = ref.mutex_span.Slice(std::span(nodes.edges));
		for (auto ite : mutex_span)
		{
			auto& ref2 = nodes.nodes[ite];
			ref2.mutex_count -= 1;
			if (TryStartSingleTaskFlowImp(context, ite))
			{
				count += 1;
			}
		}

		auto directed_span = ref.directed_span.Slice(std::span(nodes.edges));
		for (auto ite : directed_span)
		{
			auto& ref2 = nodes.nodes[ite];
			assert(ref2.current_in_degree >= 1);
			ref2.current_in_degree -= 1;
			if (TryStartSingleTaskFlowImp(context, ite))
			{
				count += 1;
			}
		}
		return count;
	}

	bool IndependenceTaskFlowExecute::Reset()
	{
		std::lock_guard lg(mutex);
		if(state == RunningState::Done)
		{
			for(auto& ite : nodes.nodes)
			{
				ite.current_in_degree = ite.require_in_degree;
				ite.status = RunningState::Idle;
			}
			run_task = 0;
			state = RunningState::Idle;
			return true;
		}
		return false;
	}

	bool IndependenceTaskFlowExecute::ReCloneNode()
	{
		if(owner)
		{
			std::lock_guard lg(mutex);
			if(state == RunningState::Done || state == RunningState::Idle)
			{
				return owner->CloneCompliedNodes(nodes, property.filter);
			}
		}
		return false;
	}

	/*
	void TaskFlow::TaskExecute(ExecuteStatus& status)
	{
		auto ptr = reinterpret_cast<TaskFlowNode*>(status.user_data[0]);
		if (ptr != nullptr)
		{
			TaskFlowStatus tf_status{
				status.context,
				status.task_property,
				status.thread_property,
				*this,
				status.user_data[1],
				status.display_name
			};
			ptr->TaskFlowNodeExecute(tf_status);
			FinishTaskFlowNode(status.context, status.user_data[1]);
			if (run_task != compiled_nodes.size())
			{
				return;
			}
		}
		else
		{
			OnBeginTaskFlow(status);
			StartupTaskFlow(status.context);
			if (run_task != compiled_nodes.size())
			{
				return;
			}
		}
		{
			std::lock_guard lg(compiled_mutex);
			running_state = RunningState::Done;
		}
		OnFinishTaskFlow(status);
	}

	bool TaskFlow::Commit(TaskContext& context, TaskProperty property, std::u8string_view display_name)
	{
		std::lock_guard lg(compiled_mutex);
		if(running_state == RunningState::Idle)
		{
			if(context.CommitTask(this, property, {0, 0}, display_name))
			{
				running_state = RunningState::Running;
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::CommitDelay(TaskContext& context, std::chrono::steady_clock::time_point time_point, TaskProperty property, std::u8string_view display_name)
	{
		std::lock_guard lg(compiled_mutex);
		if (running_state == RunningState::Idle)
		{
			if (context.CommitDelayTask(this, time_point, property, { 0, 0 }, display_name))
			{
				running_state = RunningState::Running;
				OnPostCommit();
				return true;
			}
		}
		return false;
	}

	

	

	void TaskFlow::TaskTerminal(TaskProperty property, AppendData data) noexcept
	{
		{
			std::lock_guard lg(compiled_mutex);
			auto ptr = reinterpret_cast<TaskFlowNode*>(data[0]);
			if (ptr != nullptr)
			{
				auto& ref = compiled_nodes[data[1]];
				assert(ref.status == RunningState::Running);
				ref.status = RunningState::Done;
				assert(exist_task >= 1);
				exist_task -= 1;
				run_task += 1;
			}
			if (exist_task == 0)
			{
				running_state = RunningState::Done;
			}
		}
	}

	
	*/

}