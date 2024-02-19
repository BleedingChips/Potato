module;

#include <cassert>

export module PotatoTaskFlow;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;
import PotatoTaskSystem;


export namespace Potato::Task
{

	export struct TaskFlow;

	/*
	struct TaskFlowPause
	{
		TaskFlow& owner;
		std::size_t self_index;
		TaskProperty property;

		void Continue(TaskContext& context);
	};
	*/

	struct TaskFlowStatus
	{
		Status status = Status::Normal;
		TaskContext& context;
		TaskProperty task_property;
		ThreadProperty thread_property;
		TaskFlow& owner;
		//std::size_t self_index;
		//TaskFlowPause SetPause();
	};

	struct TaskFlowNode
	{
		struct Wrapper
		{
			template<typename T>
			void AddRef(T* ptr) { ptr->AddTaskFlowNodeRef(); }
			template<typename T>
			void SubRef(T* ptr) { ptr->SubTaskFlowNodeRef(); }
		};

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlowNode, TaskFlowNode::Wrapper>;

		template<typename Fun>
		static Ptr CreateLambda(Fun&& func, std::pmr::memory_resource* resouce = std::pmr::get_default_resource()) requires(std::is_invocable_v<Fun, TaskFlowStatus&>);

	protected:

		virtual void AddTaskFlowNodeRef() const = 0;
		virtual void SubTaskFlowNodeRef() const = 0;

		virtual void TaskFlowNodeExecute(TaskFlowStatus& status) = 0;
		virtual void TaskFlowNodeTerminal() noexcept {}
		virtual bool TryLogin() { return true; };
		virtual void Logout() {}

		friend struct TaskFlow;
	};

	/*
	struct TaskFlowGraphic
	{
		struct Edge
		{
			bool is_mutex = false;
			std::size_t form;
			std::size_t to;
		};

		TaskFlowGraphic(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: edges(resource)
		{
			
		}

		void AddDependenceNode(std::size_t from, std::size_t to)
		{
			edges.emplace_back(
				false,
				from,
				to
			);
		}

		void AddMutexNode(std::size_t from, std::size_t to)
		{
			edges.emplace_back(
				true,
				from,
				to
			);
		}

		std::pmr::vector<Edge> edges;
	};
	*/
	/*
	struct TaskFlowGraphic
	{
		TaskFlowGraphic(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			: nodes(resource)
		{
			
		}

		std::optional<std::size_t> AddNode(TaskFlowNode::Ptr ptr, TaskProperty property = {});

	protected:

		struct Edge
		{
			bool is_mutex = true;
			std::size_t from;
			std::size_t to;
		};

		struct Node
		{
			TaskFlowNode::Ptr node;
			TaskProperty property;
			std::size_t fast_index;
			std::pmr::vector<Edge> edges;
		};

		std::pmr::vector<Node> nodes;
	};
	*/


	export struct TaskFlow : public TaskFlowNode, protected Task
	{

		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow, TaskFlowNode::Wrapper>;

		/*
		std::optional<std::size_t> AddStaticNode(TaskFlowNode::Ptr node, TaskProperty property = {});
		std::optional<std::size_t> AddDynamicNode(TaskFlowNode::Ptr node, TaskProperty property = {});
		*/

		static Ptr CreateDefaultTaskFlow(std::pmr::memory_resource* resource = std::pmr::get_default_resource());


		struct TempEdge
		{
			bool is_mutex = false;
			std::size_t form;
			std::size_t to;
		};

		struct Graphic
		{
			void AddDirectedEdge(std::size_t form, std::size_t to);
			void AddMutexEdge(std::size_t form, std::size_t to);

		protected:

			Graphic(std::pmr::memory_resource* resource)
				: edges(resource) {}
			std::pmr::vector<TempEdge> edges;

			friend struct TaskFlow;
		};

		struct NodeGraphic : protected Graphic
		{
			void AddToEdge(std::size_t to)
			{
				++out_degree;
				Graphic::AddDirectedEdge(std::numeric_limits<std::size_t>::max(), to);
			}
			void AddFormEdge(std::size_t form)
			{
				++in_degree;
				Graphic::AddDirectedEdge(form, std::numeric_limits<std::size_t>::max());
			}

			void AddMutexEdge(std::size_t to)
			{
				Graphic::AddMutexEdge(to, std::numeric_limits<std::size_t>::max());
			}

		protected:

			NodeGraphic(std::pmr::memory_resource* resource) : Graphic(resource) {}
			std::size_t in_degree = 0;
			std::size_t out_degree = 0;

			friend struct TaskFlow;
		};


		std::optional<std::size_t> AddNode(TaskFlowNode::Ptr node, TaskProperty property = {}, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource());

		template<typename Func>
		std::optional<std::size_t> AddNode(Func&& func, TaskFlowNode::Ptr node, TaskProperty property = {}, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<Func, NodeGraphic&>)
		{
			if(node)
			{
				NodeGraphic temp{ temp_resource };
				func(temp);
				std::lock_guard lg(flow_mutex);
				return TryAddNode(std::move(node), property, temp, temp_resource);
			}
		}

		template<typename Func>
		bool ChangeGraphic(Func&& func, std::pmr::memory_resource* temp_resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<Func, Graphic&>)
		{
			Graphic temp{ temp_resource };
			func(temp);
			std::lock_guard lg(flow_mutex);
			return TryChangeGraphic(temp);
		}

	protected:

		TaskFlow(std::pmr::memory_resource* storage_resource);

		virtual void AddTaskRef() const override{ AddTaskFlowNodeRef(); }
		virtual void SubTaskRef() const override { SubTaskFlowNodeRef(); }
		virtual void TaskFlowNodeExecute(TaskFlowStatus& status) override;
		virtual void TaskFlowNodeTerminal() noexcept override;
		virtual void TaskExecute(ExecuteStatus& status) override;
		virtual bool TryLogin() override;
		virtual void Logout() override;

		std::optional<std::size_t> TryAddNode(TaskFlowNode::Ptr node, TaskProperty property, NodeGraphic const& graphic, std::pmr::memory_resource* temp_resource);
		bool TryChangeGraphic(Graphic const& graphic);

		virtual ~TaskFlow() = default;

		enum class Status
		{
			Idle,
			SubTaskFlow,
			Waiting,
			Baked,
			Running
		};

		struct Node
		{
			TaskFlowNode::Ptr node;
			std::size_t in_degree;
			std::size_t mutex_degree;
			TaskProperty property;
		};

		struct Edge
		{
			bool is_mutex;
			std::size_t from;
			std::size_t to;
		};

		mutable std::mutex flow_mutex;
		Status status = Status::Idle;
		std::pmr::vector<Node> nodes;
		std::pmr::vector<Edge> edges;
		bool is_modified = false;

		friend struct TaskFlowNode::Wrapper;
	};

	


	/*
	struct TaskFlowGraphic
	{
		
	};
	*/




	template<typename Func>
	struct LambdaTaskFlowNode : public TaskFlowNode, protected Pointer::DefaultIntrusiveInterface
	{
		IR::MemoryResourceRecord record;
		Func func;

		void AddTaskFlowNodeRef() const override{ DefaultIntrusiveInterface::AddRef(); }
		void SubTaskFlowNodeRef() const override { DefaultIntrusiveInterface::SubRef(); }

		void Release() override
		{
			auto re = record;
			this->~LambdaTaskFlowNode();
			re.Deallocate();
		}

		void TaskFlowNodeExecute(TaskFlowStatus& status) override { func(status); }

		LambdaTaskFlowNode(IR::MemoryResourceRecord record, Func func)
			: record(record), func(std::move(func)) {}
	};

	template<typename Fun>
	auto TaskFlowNode::CreateLambda(Fun&& func, std::pmr::memory_resource* resouce) ->Ptr requires(std::is_invocable_v<Fun, TaskFlowStatus&>)
	{
		using Type = LambdaTaskFlowNode<std::remove_cvref_t<Fun>>;
		assert(resouce != nullptr);
		auto record = Potato::IR::MemoryResourceRecord::Allocate<Type>(resouce);
		if (record)
		{
			return new (record.Get()) Type{ record, std::forward<Fun>(func) };
		}
		return {};
	}




	/*
	export struct TaskFlow : public TaskFlowNode, public Task
	{
		using Ptr = Potato::Pointer::IntrusivePtr<TaskFlow>;

		TaskFlow(std::pmr::memory_resource* mr);

		bool Commit(TaskContext& context, TaskProperty property);

		bool Pause(TaskFlowNode* target);
		bool Continue(TaskContext& context, TaskProperty property, TaskFlowNode* target);

	protected:

		virtual void TaskExecute(ExecuteStatus& status) override;
		virtual void TaskFlowNodeExecute(TaskFlowStatus& status) override;

		virtual void OnStartTaskFlow(ExecuteStatus& context) {};
		virtual void OnEndTaskFlow(ExecuteStatus& context) {};

		void StartTaskFlow(TaskContext& context, TaskProperty property);
		void FinishTask(TaskContext& context, std::size_t index, TaskProperty property);

		enum class Status
		{
			Idle,
			Waiting,
			Baked,
			Running
		};

		struct Node
		{
			TaskFlowNode::Ptr node;
			std::size_t in_degree;
			std::size_t mutex_degree;
			Misc::IndexSpan<> edge_index;
		};

		struct Edge
		{
			bool is_mutex;
			std::size_t from;
			std::size_t to;
		};

		mutable std::mutex flow_mutex;
		Status status = Status::Idle;
		std::pmr::vector<Node> static_nodes;
		std::pmr::vector<Edge> static_edges;
		TaskFlow::Ptr owner;
		TaskProperty owner_property;
		//std::pmr::vector<TaskFlowPause> pause;

		friend struct Potato::Pointer::DefaultIntrusiveWrapper;
		friend struct Builder;
	};

	struct DefaultTaskFlow : public TaskFlow
	{
		
	};
	*/






}