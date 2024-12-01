module;

#include <cassert>

module PotatoTaskFlow;

namespace Potato::Task
{

	TaskFlow::TaskFlow(std::pmr::memory_resource* task_flow_resource)
			:
		 preprocess_nodes(task_flow_resource)
		, graph(task_flow_resource)
		, preprocess_mutex_edges(task_flow_resource)
		, process_nodes(task_flow_resource)
		, process_edges(task_flow_resource)
	{

	}

	TaskFlow::~TaskFlow()
	{
		{
			std::lock_guard lg(preprocess_mutex);
			preprocess_nodes.clear();
			preprocess_mutex_edges.clear();
			graph.Clear();
		}
		
		{
			std::lock_guard lg(process_mutex);
			process_edges.clear();
			process_nodes.clear();
		}
	}

	GraphNode TaskFlow::AddNode_AssumedLocked(TaskFlowNode::Ptr node, TaskFlowNodeProperty property, std::size_t append_info)
	{
		if(node)
		{
			auto t_node = graph.Add(append_info);

			if (t_node.GetIndex() < preprocess_nodes.size())
			{
				auto& ref = preprocess_nodes[t_node.GetIndex()];
				ref.node = std::move(node);
				ref.property = property;
				ref.self = t_node;
				return t_node;
			}

			preprocess_nodes.emplace_back(std::move(node), property, t_node);
			need_update = true;
			return t_node;
		}
		return {};
	}

	bool TaskFlow::AddTemporaryNode_AssumedLocked(TaskFlowNode::Ptr node, TaskFlowNodeProperty property, bool(*detect_func)(void* append_data, TaskFlowNode const&, TaskFlowNodeProperty, TemporaryNodeIndex), void* append_data)
	{
		if(node)
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

					if (ref.status != Status::DONE)
					{
						if ((*detect_func)(append_data, *ref.node, ref.property, { top, temporary_node_offset }))
						{
							if (top < temporary_node_offset && ref.status == Status::READY)
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

					if (ref.status != Status::READY)
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
				Status::READY,
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
			need_update = true;
			return true;
		}
		return false;
	}


	bool TaskFlow::Remove_AssumedLocked(GraphNode node)
	{
		if(graph.RemoveNode(node))
		{
			preprocess_nodes[node.GetIndex()].node.Reset();
			need_update = true;
			return true;
		}
		return false;
	}


	bool TaskFlow::RemoveDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to)
	{
		if(graph.RemoveEdge(from, direct_to))
		{
			need_update = true;
			return true;
		}
		return false;
	}

	bool TaskFlow::AddMutexEdge_AssumedLocked(GraphNode from, GraphNode to, EdgeOptimize optimize)
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
			need_update = true;
			return true;
		}
		return false;
	}

	bool TaskFlow::AddDirectEdge_AssumedLocked(GraphNode from, GraphNode direct_to, EdgeOptimize optimize)
	{
		if(graph.AddEdge(from, direct_to, optimize))
		{
			need_update = true;
			return true;
		}
		return false;
	}

	bool TaskFlow::Update_AssumedLocked(std::pmr::memory_resource* resource)
	{
		if(current_status == Status::DONE || current_status == Status::READY)
		{
			auto output = graph.AcyclicCheck({});
			if (output)
				return false;
			current_status = Status::READY;
			finished_task = 0;
			
			assert(!output);
			{
				std::lock_guard lg2(preprocess_mutex);
				if(need_update)
				{
					request_task = 0;
					need_update = false;
					process_nodes.clear();
					process_edges.clear();

					for(auto& ite : preprocess_nodes)
					{
						process_nodes.emplace_back(
							ite.node ? Status::READY : Status::DONE,
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
						if(ite.node)
						{
							request_task += 1;
							ite.node->Update();
						}
					}
					for(std::size_t i = 0; i < process_nodes.size(); ++i)
					{
						auto& ref = process_nodes[i];
						auto s = process_edges.size();
						for(auto& ite : preprocess_mutex_edges)
						{
							if(ite.from == i)
							{
								process_edges.emplace_back(ite.to);
							}else if(ite.to == i)
							{
								process_edges.emplace_back(ite.from);
							}
						}
						ref.mutex_edges = {s, process_edges.size()};
						s = process_edges.size();
						for(auto& ite : graph.GetEdges())
						{
							if(ite.from == i)
							{
								auto& ref = process_nodes[ite.to];
								ref.in_degree += 1;
								ref.init_in_degree += 1;
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

			process_nodes.resize(temporary_node_offset);
			process_edges.resize(temporary_edge_offset);
			for(auto& ite : process_nodes)
			{
				if(ite.node)
				{
					ite.status = Status::READY;
					ite.in_degree = ite.init_in_degree;
					ite.mutex_degree = 0;
					ite.need_append_mutex = false;
					ite.node->Update();
				}
				
			}
			append_direct_edge.clear();
			
			return true;
		}
		return false;
	}

	bool TaskFlow::Commited_AssumedLocked(TaskContext& context, TaskFlowNodeProperty property)
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
			if(finished_task == request_task)
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
		default:
			{
				break;
			}
		}
	}
}