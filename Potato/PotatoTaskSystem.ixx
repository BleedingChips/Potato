module;

#include <cassert>

export module Potato.TaskSystem;


export import Potato.STD;
export import Potato.Misc;
export import Potato.SmartPtr;

namespace Potato::Task
{

	export struct TaskSystem;
	struct Task;

	struct TaskSystemPointerWrapper
	{
		TaskSystemPointerWrapper(TaskSystemPointerWrapper const&) = default;
		TaskSystemPointerWrapper(TaskSystemPointerWrapper&&) = default;
		TaskSystemPointerWrapper() = default;

		inline static void AddRef(TaskSystem* t);
		inline static void SubRef(TaskSystem* t);
	};

	export using TaskSystemPtr = Potato::Misc::IntrusivePtr<TaskSystem, TaskSystemPointerWrapper>;

	export enum class TaskPriority
	{
		VeryHigh,
		High,
		Normal,
		Low,
		VeryLow,
	};

	export enum class TaskGuessConsume
	{
		VeryHigh,
		High,
		Normal,
		Low,
		VeryLow,
	};

	struct TaskProperty
	{
		std::wstring_view Name = L"UnDefine";
		TaskPriority Priority = TaskPriority::Normal;
		TaskGuessConsume Consume = TaskGuessConsume::Normal;
	};


	struct TaskInterfaceWeakPointerWrapper;

	struct TaskInterfaceStrongPointerWrapper;

	struct TaskInterface
	{

		TaskProperty GetProperty() const { return Property;  }

		std::wstring_view GetTaskName() const { return TaskName; }
		TaskPriority GetTaskPriority() const { return Priority; }
		TaskGuessConsume GetTaskGuessConsume() const { return Consume; }

	protected:

		virtual void ReleaseTask() const = 0;
		virtual void ReleaseSelf() const = 0;

		TaskInterface(TaskProperty Pro) : Property(Pro) {}

	private:

		virtual bool IsReady() const { return true; }
		virtual bool IsMulityTask() const { return false; }
		virtual bool IsReCommit() const { return false; }

		void AddSRef() const;
		void AddWRef() const;
		void SubSRef() const;
		void SubWRef() const;

		mutable Misc::AtomicRefCount SRef;
		mutable Misc::AtomicRefCount WRef;

		TaskProperty const Property;

		friend struct TaskInterfaceWeakPointerWrapper;
		friend struct TaskInterfaceStrongPointerWrapper;
		friend struct TaskSystem;
	};

	struct TaskInterfaceStrongPointerWrapper
	{
		using SwitchT = TaskInterfaceWeakPointerWrapper;
		TaskInterfaceStrongPointerWrapper(TaskInterfaceWeakPointerWrapper const&) {};

		inline static void AddRef(TaskInterface const* Ptr) { assert(Ptr != nullptr); Ptr->AddSRef(); }
		inline static void SubRef(TaskInterface const* Ptr) { assert(Ptr != nullptr); Ptr->SubSRef(); }
	};

	struct TaskInterfaceWeakPointerWrapper
	{
		using SwitchT = TaskInterfaceStrongPointerWrapper;
		TaskInterfaceWeakPointerWrapper(TaskInterfaceStrongPointerWrapper const&) {};
		using ForbidPtrT = void;

		inline static void AddRef(TaskInterface const* Ptr) { assert(Ptr != nullptr); Ptr->AddWRef(); }
		inline static void SubRef(TaskInterface const* Ptr) { assert(Ptr != nullptr); Ptr->SubWRef(); }
	};

	export using TaskPtr = Misc::SmartPtr<TaskInterface, TaskInterfaceStrongPointerWrapper>;

	export struct TaskSystem
	{

		static TaskSystemPtr Create(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1, Allocator<TaskSystem> Allo= Allocator<TaskSystem>{DefaultMemoryResource()});
		template<typename FuncT>
		TaskPtr CreateLambdaTask(TaskProperty Property, FuncT Func, MemoryResource* Resource = DefaultMemoryResource());

	protected:

		void AddRef();
		void SubRef();

		~TaskSystem();
		TaskSystem(std::size_t ThreadCount, Allocator<TaskSystem> RequireAllocator);

		void Executor();

		std::atomic_bool Available = true;
		Misc::AtomicRefCount Ref;
		Allocator<TaskSystem> SelfAllocator;
		std::vector<std::thread, Allocator<std::thread>> Thread;

		friend struct TaskSystemPointerWrapper;
	};

	void TaskSystemPointerWrapper::AddRef(TaskSystem* t)
	{
		assert(t != nullptr);
		t->AddRef();
	}

	void TaskSystemPointerWrapper::SubRef(TaskSystem* t)
	{
		assert(t != nullptr);
		t->SubRef();
	}

	template<typename T> 
	struct NormalTaskImplement : public TaskInterface, protected T
	{
		template<typename ...AT>
		TaskImplement(TaskProperty Property, AllocatorT<NormalTaskImplement<T>> Allo, AT ...Ats)
			: TaskInterface(Property), T(std::forward<AT>(Ats)...), SelfAllocator(Allo)
		{}

		AllocatorT<NormalTaskImplement<T>> SelfAllocator;
	};

	template<typename FuncT>
	TaskPtr TaskSystem::CreateLambdaTask(TaskProperty Property, FuncT Func, MemoryResource* Resource)
	{
		
	}
}