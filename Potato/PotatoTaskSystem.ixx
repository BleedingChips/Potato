module;

#include <cassert>

export module PotatoTaskSystem;

import std;
import PotatoMisc;
import PotatoPointer;

export namespace Potato::Task
{
	struct ControlPtrDefaultWrapper
	{
		template<typename PtrT>
		void AddRef(PtrT* Ptr) { Ptr->AddRef(); Ptr->AddControlRef(); }
		template<typename PtrT>
		void SubRef(PtrT* Ptr) { Ptr->SubControlRef(); Ptr->SubRef(); }
	};

	struct ControlDefaultInterface : public Potato::Pointer::DefaultIntrusiveInterface
	{

		bool IsControlRefZero() const { return CRef.Count() == 0; }

	protected:

		void AddControlRef() const { CRef.AddRef(); }
		void SubControlRef() const { if (CRef.SubRef()) const_cast<ControlDefaultInterface*>(this)->ControlRelease(); }
		virtual void ControlRelease() = 0;
		mutable Misc::AtomicRefCount CRef;
		friend struct ControlPtrDefaultWrapper;
	};

}

export namespace Potato::Task
{

	template<typename PtrT>
	using ControlPtr = Potato::Pointer::SmartPtr<PtrT, Pointer::SmartPtrDefaultWrapper<ControlPtrDefaultWrapper>>;

	enum class TaskPriority : std::size_t
	{
		High = 100,
		Normal = 1000,
		Low = 10000,
	};

	constexpr std::size_t operator*(TaskPriority Priority) {
		return static_cast<std::size_t>(Priority);
	}

	struct TaskContext;

	struct TaskProperty
	{
		std::size_t TaskPriority = *TaskPriority::Normal;
		std::u8string_view TaskName = u8"Unnamed Task";
		std::size_t AppendData = 0;
		std::size_t AppendData2 = 0;
	};

	enum class TaskContextStatus : std::size_t
	{
		Normal,
		Close,
	};

	struct ExecuteStatus
	{
		TaskContextStatus Status = TaskContextStatus::Normal;
		TaskProperty Property;
		TaskContext& Context;

		operator bool() const { return Status == TaskContextStatus::Normal; }
	};

	struct Task : public ControlDefaultInterface
	{
		using Ptr = ControlPtr<Task>;

		template<typename FunT>
		static Ptr CreatLambdaTask(FunT&& Func, std::pmr::memory_resource* Resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, ExecuteStatus& , Task::Ptr>)
		;

	protected:

		using WPtr = Pointer::IntrusivePtr<Task>;

		virtual void ControlRelease() override {};
		virtual void operator()(ExecuteStatus& Status) = 0;
		virtual ~Task() = default;

		friend struct TaskContext;
	};

	struct TaskContext : public ControlDefaultInterface
	{
		using Ptr = ControlPtr<TaskContext>;

		static Ptr Create(std::pmr::memory_resource* Resource = std::pmr::get_default_resource());
		bool FireThreads(std::size_t TaskCount = std::thread::hardware_concurrency() - 1);

		std::size_t CloseThreads();
		void FlushTask();
		void WaitTask();

		bool CommitTask(Task::Ptr TaskPtr, TaskProperty Property = {});
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::time_point TimePoint, TaskProperty Property = {});
		bool CommitDelayTask(Task::Ptr TaskPtr, std::chrono::system_clock::duration Duration, TaskProperty Property = {})
		{
			return CommitDelayTask(std::move(TaskPtr), std::chrono::system_clock::now() + Duration, Property);
		}

		~TaskContext();

	private:

		void Release();
		void ControlRelease();

		using WPtr = Pointer::IntrusivePtr<TaskContext>;

		TaskContext(std::pmr::memory_resource* Resource);

		struct ExecuteContext
		{
			std::size_t LastingTask = 1;
			bool LastExecute = false;
			bool Locked = false;
			TaskContextStatus Status = TaskContextStatus::Normal;
		};

		void ExecuteOnce(ExecuteContext& Context, std::chrono::system_clock::time_point CurrentTime);

		std::pmr::memory_resource* Resource;

		std::mutex ThreadMutex;
		std::pmr::vector<std::jthread> Threads;

		struct ReadyTaskT
		{
			TaskProperty Property;
			Task::WPtr Task;
		};

		struct DelayTaskT
		{
			std::chrono::system_clock::time_point DelayTimePoint;
			TaskProperty Property;
			Task::WPtr Task;
		};

		std::mutex TaskMutex;
		TaskContextStatus Status = TaskContextStatus::Normal;
		std::size_t LastingTask = 0;
		std::chrono::system_clock::time_point ClosestTimePoint;
		std::pmr::vector<DelayTaskT> DelayTasks;
		std::pmr::vector<ReadyTaskT> ReadyTasks;
	};


	struct DependenceTaskGraphic : protected Task
	{

		struct Node : public Pointer::DefaultIntrusiveInterface
		{
			using Ptr = Pointer::IntrusivePtr<Node>;

			virtual ~Node() = default;
		protected:
			virtual void Release() = 0;
			Node() = default;
		};

		struct Builder
		{
		protected:
			Builder(DependenceTaskGraphic& Graphic)
				: Graphic(Graphic) {}
			DependenceTaskGraphic& Graphic;

			friend DependenceTaskGraphic;
		};

		template<typename FunT>
		bool CreateGraphic(FunT const& Func)
			requires(std::is_invocable_v<FunT, Builder&>)
		{
			std::lock_guard lg(NodeMutex);
			if(Nodes.empty())
			{
				Builder Build{ *this };
				Func(Build);
				return true;
			}
			return false;
		}

	protected:

		DependenceTaskGraphic(TaskContext::Ptr Owner)
			: Owner(Owner) {}

		virtual void operator()(ExecuteStatus& Status) final override;

		TaskContext::Ptr Owner;

		struct NormalNode
		{
			TaskProperty Property;
			Node::Ptr Node;
			bool Done = false;
		};

		struct Dependence
		{
			std::size_t Require;
			//std::size_t 
		};

		std::mutex NodeMutex;

		std::pmr::vector<NormalNode> Nodes;
		//std::pmr::vector<>
	};


}

namespace Potato::Task
{

	template<typename TaskImpT>
	struct TaskImp : public Task
	{

		virtual void Release() override
		{
			auto OldResource = Resource;
			this->~TaskImp();
			OldResource->deallocate(const_cast<TaskImp*>(this), sizeof(TaskImp), alignof(TaskImp));
		}

		template<typename FunT>
		TaskImp(std::pmr::memory_resource* Resource, FunT&& Fun)
			: Resource(Resource), TaskInstance(std::forward<FunT>(Fun))
		{
			
		}

		virtual void operator()(ExecuteStatus& Status) override
		{
			Task::Ptr ThisPtr{this};
			TaskInstance.operator()(Status, std::move(ThisPtr));
		}

	private:

		std::pmr::memory_resource* Resource;
		TaskImpT TaskInstance;
	};

	template<typename FunT>
	Task::Ptr Task::CreatLambdaTask(FunT&& Func, std::pmr::memory_resource* Resource)
		requires(std::is_invocable_v<FunT, ExecuteStatus&, Task::Ptr>)
	{
		using Type = TaskImp<std::remove_cvref_t<FunT>>;
		assert(Resource != nullptr);
		auto Adress = Resource->allocate(sizeof(Type), alignof(Type));
		if(Adress != nullptr)
		{
			return new (Adress) Type{ Resource, std::forward<FunT>(Func)};
		}
		return {};
	}
}