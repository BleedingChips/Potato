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
		void AddTaskFlowNodeRef() const override { DefaultIntrusiveInterface::AddRef(); }
		void SubTaskFlowNodeRef() const override { DefaultIntrusiveInterface::SubRef(); }
		void Release() override
		{
			auto re = record;
			this->~DefaultTaskFlow();
			re.Deallocate();
		}
	};

	std::optional<std::size_t> TaskFlow::AddStaticNode(TaskFlowNode::Ptr node, TaskProperty property)
	{
		std::lock_guard lg(flow_mutex);
		if(status == Status::Idle && node)
		{
			auto size = nodes.size();
			nodes.emplace_back(
				std::move(node),
				0,
				0,
				property
			);
			return size;
		}
		return std::nullopt;
	}

	auto TaskFlow::CreateDefaultTaskFlow(std::pmr::memory_resource* resource)->Ptr
	{
		auto re = Potato::IR::MemoryResourceRecord::Allocate<DefaultTaskFlow>(resource);
		if(re)
		{
			return new(re.Get()) DefaultTaskFlow{re};
		}
		return {};
	}

	void TaskFlow::TaskExecute(ExecuteStatus& status)
	{
		
	}

	void TaskFlow::TaskFlowNodeExecute(TaskFlowStatus& status)
	{
		
	}

	void TaskFlow::TaskFlowNodeTerminal() noexcept
	{
		
	}

	bool TaskFlow::TryLogin()
	{
		std::lock_guard lg(flow_mutex);
		if(status == Status::Idle)
		{
			status = Status::SubTaskFlow;
			return true;
		}
		return false;
	}

	void TaskFlow::Logout()
	{
		std::lock_guard lg(flow_mutex);
		assert(status == Status::SubTaskFlow);
		status = Status::Idle;
	}

	TaskFlow::TaskFlow(std::pmr::memory_resource* storage_resource)
		: nodes(storage_resource), edges(storage_resource)
	{
		/*
		nodes.reserve(graphic.nodes.size());
		std::size_t index = 0;
		for(auto& ite : graphic.nodes)
		{
			nodes.push_back(
			
			);
			index += ite.edges.size();
		}
		edges.reserve(index);

		std::pmr::monotonic_buffer_resource mbr(storage_resource);

		*/


	}






	/*
	void TaskFlowNode::TaskExecute(ExecuteStatus& status) override
	{
		auto new_ad = DecodeAppendDataForTaskFlow(
			status.user_data
		);

	}
	*/



	/*
	bool TaskFlow::Commit(TaskContext& context, TaskProperty property)
	{
		std::lock_guard lg(flow_mutex);
		if(status == Status::Idle)
		{
			status = Status::Running;
			if(context.CommitTask(this, property, {0, 0}))
			{
				return true;
			}
		}
		return false;
	}

	void TaskFlow::TaskExecute(ExecuteStatus& status)
	{
		auto& ud = status.user_data;
		auto ptr = reinterpret_cast<TaskFlowNode*>(ud[0]);
		if(ptr == nullptr)
		{
			if(ud[1] == 0)
			{
				OnStartTaskFlow(status);
				StartTaskFlow(status.context, status.task_property);
			}else if(ud[1] == 1)
			{
				OnEndTaskFlow(status);
				{
					std::lock_guard lg(flow_mutex);
					if(owner)
					{
						auto new_owner = std::move(owner);
						new_owner->Continue(status.context, status.task_property, this);
					}
				}
			}else
			{
				assert(false);
			}
		}else
		{
			TaskFlowStatus TFS{
				status.status,
				status.context,
				status.task_property,
				status.thread_property,
				*this,
			};

			ptr->TaskFlowNodeExecute(TFS);

			FinishTask(status.context, ud[1], status.task_property);
		}
	}

	void TaskFlow::TaskFlowNodeExecute(TaskFlowStatus& status)
	{
		{
			std::lock_guard lg(flow_mutex);
			owner = &status.owner;
			owner_property = status.task_property;
		}

		AppendData data;
		data[0] = 0;
		data[1] = 0;

		status.context.CommitTask(
			this,
			status.task_property,
			data
		);
	}
	*/


}