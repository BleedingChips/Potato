module;

#include <cassert>

module PotatoTaskFlow;

namespace Potato::Task
{

	struct DefaultTaskFlow : public TaskFlow, protected Pointer::DefaultIntrusiveInterface
	{
		DefaultTaskFlow(Potato::IR::MemoryResourceRecord record)
			: TaskFlow(record.GetResource()), record(record) {}

	protected:

		Potato::IR::MemoryResourceRecord record;
		void AddTaskRef() const override { DefaultIntrusiveInterface::AddRef(); }
		void SubTaskRef() const override { DefaultIntrusiveInterface::SubRef(); }

		void Release() override
		{
			auto re = record;
			this->~DefaultTaskFlow();
			re.Deallocate();
		}
	};

	TaskFlow::TaskFlow(std::pmr::memory_resource* resource)
		: temp_resource(resource), pre_compiled_nodes(resource), compiled_nodes(resource), compiled_edges(resource)
	{
		
	}

	auto TaskFlow::CreateDefaultTaskFlow(std::pmr::memory_resource* resource)->Ptr
	{
		auto re = Potato::IR::MemoryResourceRecord::Allocate<DefaultTaskFlow>(resource);
		if (re)
		{
			return new(re.Get()) DefaultTaskFlow{ re };
		}
		return {};
	}

	bool TaskFlow::AddDirectEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to)
	{
		if(form && direct_to)
		{
			std::lock_guard lg(pre_compiled_mutex);
			std::lock(form->node_mutex, direct_to->node_mutex);
			std::lock_guard lg2(form->node_mutex, std::adopt_lock);
			std::lock_guard lg3(direct_to->node_mutex, std::adopt_lock);
			if (form->owner == this && direct_to->owner == this && form->fast_index != direct_to->fast_index)
			{
				auto& ref1 = pre_compiled_nodes[form->fast_index];
				auto& ref2 = pre_compiled_nodes[direct_to->fast_index];
				if(!ref1.require_remove && !ref2.require_remove)
				{
					ref1.edges.emplace_back(
						EdgeType::Direct,
						direct_to.GetPointer()
					);
					ref1.out_degree += 1;
					ref1.edge_count += 1;
					ref2.edges.emplace_back(
						EdgeType::ReverseDirect,
						form.GetPointer()
					);
					ref2.in_degree += 1;
					need_update = true;
					return true;
				}
			}
		}
		
		return false;
	}

	bool TaskFlow::AddMutexEdges(TaskFlowNode::Ptr form, TaskFlowNode::Ptr direct_to)
	{
		if(form && direct_to)
		{
			std::lock_guard lg(pre_compiled_mutex);
			std::lock(form->node_mutex, direct_to->node_mutex);
			std::lock_guard lg2(form->node_mutex, std::adopt_lock);
			std::lock_guard lg3(direct_to->node_mutex, std::adopt_lock);
			if (form->owner == this && direct_to->owner == this && form->fast_index != direct_to->fast_index)
			{
				auto& ref1 = pre_compiled_nodes[form->fast_index];
				auto& ref2 = pre_compiled_nodes[direct_to->fast_index];
				if (!ref1.require_remove && !ref2.require_remove)
				{
					ref1.edges.emplace_back(
						EdgeType::Mutex,
						direct_to.GetPointer()
					);
					ref1.edge_count += 1;
					ref2.edges.emplace_back(
						EdgeType::Mutex,
						form.GetPointer()
					);
					ref2.edge_count += 1;
					need_update = true;
					return true;
				}
			}
		}
		return false;
	}

	bool TaskFlow::ResetState()
	{
		std::lock_guard lg(compiled_mutex);
		if(running_state == RunningState::Done)
		{
			running_state = RunningState::Idle;
			for(auto& ite : compiled_nodes)
			{
				ite.status = RunningState::Idle;
			}
			exist_task = 0;
			run_task = 0;
			return true;
		}
		return false;
	}

	bool TaskFlow::Update(bool reset_state, std::pmr::vector<TaskFlowNode::Ptr>* error_output, std::pmr::memory_resource* temp)
	{
		bool HasUpdate = false;
		{
			std::lock_guard lg(pre_compiled_mutex);
			if (need_update && running_state != RunningState::Running)
			{
				HasUpdate = true;
				need_update = false;
				if (require_remove_count != 0)
				{
					//require_remove_node.reserve(require_remove_count);
					require_remove_count = 0;
					for (std::size_t i = 0; i < pre_compiled_nodes.size();)
					{
						auto& ref = pre_compiled_nodes[i];
						if (ref.require_remove)
						{
							auto node = std::move(ref.node);
							auto pro = RemoveNodeImp(*node);
							//require_remove_node.emplace_back(std::move(node), pro);
						}
						else
						{
							++i;
						}
					}
				}

				std::lock_guard lg(compiled_mutex);
				compiled_nodes.clear();
				compiled_edges.clear();

				enum class TouchState
				{
					UnTouch,
					FirstTouch,
					SecondTouch,
				};

				struct UpdateNode
				{
					TaskFlowNode::Ptr node;
					std::size_t in_degree;
					TouchState touch_state = TouchState::UnTouch;
					std::optional<std::size_t> compiled_node_mapping;
					Misc::IndexSpan<> mutex_edge;
					Misc::IndexSpan<> direct_edge;
				};

				std::pmr::vector<UpdateNode> u_nodes(temp);
				std::pmr::vector<std::size_t> u_edges(temp);
				std::pmr::vector<std::size_t> search_stack(temp);

				u_nodes.reserve(pre_compiled_nodes.size());
				compiled_nodes.reserve(pre_compiled_nodes.size());

				{
					std::size_t edge_total_count = 0;
					for (auto& ite : pre_compiled_nodes)
					{
						edge_total_count += ite.edge_count;
						ite.updated = true;
					}
					u_edges.reserve(edge_total_count);
				}

				for (auto& ite : pre_compiled_nodes)
				{
					UpdateNode new_node;
					new_node.node = ite.node;
					new_node.in_degree = ite.in_degree;
					auto old_size = u_edges.size();

					for (auto& ite2 : ite.edges)
					{
						if (ite2.type == EdgeType::Mutex)
						{
							u_edges.push_back(ite2.node->GetFastIndex());
						}
					}

					new_node.mutex_edge = { old_size, u_edges.size() };

					old_size = u_edges.size();

					for (auto& ite2 : ite.edges)
					{
						if (ite2.type == EdgeType::Direct)
						{
							u_edges.push_back(ite2.node->GetFastIndex());
						}
					}

					new_node.direct_edge = { old_size, u_edges.size() };

					u_nodes.emplace_back(
						std::move(new_node)
					);

				}


				while (compiled_nodes.size() < u_nodes.size())
				{
					std::optional<std::size_t> target_node;
					for (std::size_t i = 0; i < u_nodes.size(); ++i)
					{
						if (u_nodes[i].in_degree == 0 && !u_nodes[i].compiled_node_mapping.has_value())
						{
							target_node = i;
							break;
						}
					}

					if (target_node.has_value())
					{
						auto new_ref_node = compiled_nodes.size();
						auto& pnode = pre_compiled_nodes[*target_node];
						auto& unode = u_nodes[*target_node];
						unode.compiled_node_mapping = new_ref_node;

						CompiledNode new_node;
						new_node.ptr = pnode.node;
						new_node.property = pnode.property;
						auto old_edge_node = compiled_edges.size();

						auto mutex_span = unode.mutex_edge.Slice(std::span(u_edges));

						for (auto ite : mutex_span)
						{
							auto span = std::span(compiled_edges).subspan(old_edge_node);

							if (std::find(span.begin(), span.end(), ite) == span.end())
								compiled_edges.emplace_back(ite);
						}

						new_node.mutex_span = { old_edge_node, compiled_edges.size() };

						auto edge_span = unode.direct_edge.Slice(std::span(u_edges));

						for (auto ite : edge_span)
						{
							assert(u_nodes[ite].in_degree >= 1);
							u_nodes[ite].in_degree -= 1;
						}

						if (!edge_span.empty())
						{
							for (auto& ite : u_nodes)
							{
								ite.touch_state = TouchState::UnTouch;
							}

							unode.touch_state = TouchState::SecondTouch;

							search_stack.clear();

							search_stack.push_back(*target_node);

							while (!search_stack.empty())
							{
								auto top = *search_stack.rbegin();
								search_stack.pop_back();

								auto edge_span = u_nodes[top].direct_edge.Slice(std::span(u_edges));
								bool first_touch = (top == *target_node);

								for (auto ite2 : edge_span)
								{
									auto& ref = u_nodes[ite2];
									if (!first_touch)
									{
										switch (ref.touch_state)
										{
										case TouchState::UnTouch:
											ref.touch_state = TouchState::SecondTouch;
											search_stack.push_back(ite2);
											break;
										case TouchState::FirstTouch:
											ref.touch_state = TouchState::SecondTouch;
											break;
										}
									}
									else
									{
										if (ref.touch_state == TouchState::UnTouch)
										{
											ref.touch_state = TouchState::FirstTouch;
											search_stack.push_back(ite2);
										}
									}
								}

							}

							auto edge_old_size = compiled_edges.size();

							for (std::size_t i = 0; i < u_nodes.size(); ++i)
							{
								if (u_nodes[i].touch_state == TouchState::FirstTouch)
								{
									compiled_edges.push_back(i);
								}
							}

							new_node.directed_span = { edge_old_size, compiled_edges.size() };
						}

						compiled_nodes.push_back(new_node);
					}
					else
					{
						compiled_nodes.clear();
						compiled_edges.clear();
						if (error_output != nullptr)
						{
							for (std::size_t i = 0; i < u_nodes.size(); ++i)
							{
								if (u_nodes[i].in_degree != 0 && !u_nodes[i].compiled_node_mapping.has_value())
								{
									assert(u_nodes[i].compiled_node_mapping.has_value());
									error_output->push_back(pre_compiled_nodes[i].node);
									break;
								}
							}
							return false;
						}

					}
				}


				for (auto& ite : compiled_edges)
				{
					assert(u_nodes[ite].compiled_node_mapping.has_value());
					ite = *u_nodes[ite].compiled_node_mapping;
				}

				for (auto& ite : compiled_nodes)
				{
					auto span = ite.directed_span.Slice(std::span(compiled_edges));
					for (auto ite2 : span)
					{
						auto& ref = compiled_nodes[ite2];
						ref.require_in_degree += 1;
						ref.current_in_degree = ref.require_in_degree;
					}
				}

				if(running_state == RunningState::Done && reset_state)
				{
					running_state = RunningState::Idle;
					exist_task = 0;
					run_task = 0;
				}
			}else if(running_state == RunningState::Done && reset_state)
			{
				running_state = RunningState::Idle;
				exist_task = 0;
				run_task = 0;
				for(auto& ite : compiled_nodes)
				{
					ite.status = RunningState::Idle;
				}
			}
		}
		/*
		for(auto& ite : require_remove_node)
		{
			ite.node->TaskFlowNodeTerminal(ite.property);
		}
		*/
		return HasUpdate;
	}

	bool TaskFlow::AddNode(TaskFlowNode::Ptr ptr, TaskProperty property)
	{
		if(ptr)
		{
			std::lock(pre_compiled_mutex, ptr->node_mutex);
			std::lock_guard lg1(pre_compiled_mutex, std::adopt_lock);
			std::lock_guard lg2(ptr->node_mutex, std::adopt_lock);

			if(ptr->owner == nullptr)
			{
				ptr->owner = this;
				ptr->fast_index = pre_compiled_nodes.size();
				pre_compiled_nodes.emplace_back(
					std::move(ptr),
					property,
					std::pmr::vector<PreCompiledEdge>{&temp_resource},
					0,
					0,
					0,
					false,
					false
				);
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::Remove(TaskFlowNode::Ptr form)
	{
		if(form)
		{
			std::optional<TaskProperty> property;
			{
				std::lock(pre_compiled_mutex, form->node_mutex);
				std::lock_guard lg(pre_compiled_mutex, std::adopt_lock);
				std::lock_guard lg2(form->node_mutex, std::adopt_lock);
				if (form->owner == this)
				{
					auto fi = form->fast_index;
					assert(fi <= pre_compiled_nodes.size());
					auto& ref = pre_compiled_nodes[fi];
					if (!ref.updated)
					{
						property = RemoveNodeImp(*form);
					}
					else
					{
						ref.require_remove = true;
						++require_remove_count;
					}
					need_update = true;
				}
			}
			if(property.has_value())
			{
				form->TaskFlowNodeTerminal(*property);
			}
		}
		return false;
	}

	TaskProperty TaskFlow::RemoveNodeImp(TaskFlowNode& node)
	{
		assert(node.owner == this && node.fast_index < pre_compiled_nodes.size());
		auto fi = node.fast_index;
		for(std::size_t i = fi + 1; i < pre_compiled_nodes.size(); ++i)
		{
			auto& ref = pre_compiled_nodes[i];
			{
				std::lock_guard lg(ref.node->node_mutex);
				ref.node->fast_index -= 1;
			}
		}
		auto& ref = pre_compiled_nodes[fi];
		for(auto& ite : ref.edges)
		{
			auto rid = ite.node->GetFastIndex();
			auto& refid = pre_compiled_nodes[ite.node->GetFastIndex()];
			auto find = std::find_if(
			refid.edges.begin(), refid.edges.end(), 
			[&](PreCompiledEdge const& ite)
			{
				return ite.node.GetPointer() == &node;
			});

			assert(find != refid.edges.end());

			refid.edges.erase(find);
		}
		auto pro = ref.property;
		pre_compiled_nodes.erase(pre_compiled_nodes.begin() + fi);
		node.owner = nullptr;
		node.fast_index = std::numeric_limits<std::size_t>::max();
		return pro;
	}

	TaskFlow::~TaskFlow()
	{
		std::lock_guard lg(pre_compiled_mutex);
		pre_compiled_nodes.clear();
		need_update = true;

		std::lock_guard lg2(compiled_mutex);
		compiled_nodes.clear();
		compiled_edges.clear();
	}

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
				status.user_data[1]
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
		OnFinishTaskFlow(status);
		{
			std::lock_guard lg(compiled_mutex);
			running_state = RunningState::Done;
		}
	}

	bool TaskFlow::Commit(TaskContext& context, TaskProperty property)
	{
		std::lock_guard lg(compiled_mutex);
		if(running_state == RunningState::Idle)
		{
			if(context.CommitTask(this, property, {0, 0}))
			{
				running_state = RunningState::Running;
				return true;
			}
		}
		return false;
	}

	bool TaskFlow::TryStartSingleTaskFlowImp(TaskContext& context, std::size_t fast_index)
	{
		assert(fast_index < compiled_nodes.size());
		auto& ref = compiled_nodes[fast_index];
		assert(ref.ptr);
		if (ref.current_in_degree == 0 && ref.mutex_count == 0 && ref.status == RunningState::Idle)
		{
			if (context.CommitTask(
				this,
				ref.property,
				{ reinterpret_cast<std::size_t>(ref.ptr.GetPointer()), fast_index }
			))
			{
				auto mutex_span = ref.mutex_span.Slice(std::span(compiled_edges));
				for (auto ite : mutex_span)
				{
					compiled_nodes[ite].mutex_count += 1;
				}
				ref.status = RunningState::Running;
				exist_task += 1;
				return true;
			}
		}
		return false;
	}

	std::size_t TaskFlow::StartupTaskFlow(TaskContext& context)
	{
		std::lock_guard lg(compiled_mutex);
		std::size_t count = 0;
		for(std::size_t i = 0; i < compiled_nodes.size(); ++i)
		{
			if(TryStartSingleTaskFlowImp(context, i))
			{
				count += 1;
			}
		}
		return count;
	}

	void TaskFlow::TaskTerminal(TaskProperty property, AppendData data) noexcept
	{
		std::lock_guard lg(compiled_mutex);
		auto ptr = reinterpret_cast<TaskFlowNode*>(data[0]);
		if(ptr != nullptr)
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

	std::size_t TaskFlow::FinishTaskFlowNode(TaskContext& context, std::size_t fast_index)
	{
		std::size_t count = 0;
		std::lock_guard lg(compiled_mutex);
		assert(exist_task >= 1);
		exist_task -= 1;
		run_task += 1;
		assert(fast_index < compiled_nodes.size());
		auto& ref = compiled_nodes[fast_index];

		auto mutex_span = ref.mutex_span.Slice(std::span(compiled_edges));
		for (auto ite : mutex_span)
		{
			auto& ref2 = compiled_nodes[ite];
			ref2.mutex_count -= 1;
			if(TryStartSingleTaskFlowImp(context, ite))
			{
				count += 1;
			}
		}
		
		auto directed_span = ref.directed_span.Slice(std::span(compiled_edges));
		for (auto ite : directed_span)
		{
			auto& ref2 = compiled_nodes[ite];
			assert(ref2.current_in_degree >= 1);
			ref2.current_in_degree -= 1;
			if(TryStartSingleTaskFlowImp(context, ite))
			{
				count += 1;
			}
		}
		return count;
	}

}