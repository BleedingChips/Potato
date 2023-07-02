module;

#include <cassert>

export module Potato.TaskSystem;

export import Potato.STD;
export import Potato.Misc;
export import Potato.SmartPtr;

namespace Potato::Task
{
	struct Task;

	struct TaskSystem;


	struct TaskSystemReference
	{
		Misc::AtomicRefCount SRef;
		Misc::AtomicRefCount WRef;
		std::pmr::polymorphic_allocator<> Allocator;
		std::pmr::synchronized_pool_resource MemoryPool;

		TaskSystemReference(std::pmr::polymorphic_allocator<> IAllocator, std::pmr::memory_resource* MemoryResouce)
			: Allocator(IAllocator), MemoryPool(MemoryResouce)
		{

		}

		void AddStrongRef(TaskSystem* Ptr);
		void SubStrongRef(TaskSystem* Ptr);
		void AddWeakRef(TaskSystem* Ptr);
		void SubWeakRef(TaskSystem* Ptr);
		bool TryAddStrongRef(TaskSystem* Ptr);
	};

}



export namespace Potato::Task
{

	struct TaskGraph;


	struct TaskSystem
	{

		using Ptr = Misc::StrongPtr<TaskSystem, TaskSystemReference>;
		using WeakPtr = Misc::WeakPtr<TaskSystem, TaskSystemReference>;

		static Ptr Create(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1, std::pmr::memory_resource* MemoryResouce = std::pmr::get_default_resource());

	protected:
		
		~TaskSystem();
		TaskSystem(std::size_t ThreadCount, std::pmr::memory_resource* MemoryResource, TaskSystemReference* Ref);

		void Executor();

		std::atomic_bool Available = true;
		TaskSystemReference* Ref = nullptr;
		std::vector<std::thread, std::pmr::polymorphic_allocator<std::thread>> Thread;

		friend struct TaskSystemReference;
	};

	/*
	struct TaskGraph
	{
		virtual std::wstring_view GetGraphName() = 0;
	};




	struct TaskInterface
	{

		TaskProperty GetProperty() const { return Property;  }

	protected:

		virtual void ReleaseTask() const = 0;
		virtual void ReleaseSelf() const = 0;

		TaskInterface(TaskProperty Pro, TaskSystemPtr ) : Property(Pro) {}

	private:

		virtual bool IsReady() const { return true; }
		virtual bool IsMulityTask() const { return false; }
		virtual bool IsReCommit() const { return false; }
		virtual void Execute(TaskSystem& System) const = 0;

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

	export using TaskPtr = Misc::SmartPtr<TaskInterface, TaskInterfaceStrongPointerWrapper>;

	export struct TaskSystem
	{

		static TaskSystemPtr Create(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1, Allocator<TaskSystem> Allo= Allocator<TaskSystem>{DefaultMemoryResource()});
		
		template<typename FuncT>
		TaskPtr CreateLambdaTask(TaskProperty Property, FuncT&& Func, MemoryResource* Resource = DefaultMemoryResource());

		void AddRef();
		void SubRef();

	protected:

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
	//requires(std::is_onvoideable_v<T, TaskSystem&>)
	struct NormalTaskImplement : public TaskInterface
	{
		template<typename ...AT>
		NormalTaskImplement(TaskProperty Property, Allocator<NormalTaskImplement<T>> Allo, AT&& ...Ats)
			: TaskInterface(Property), SelfAllocator(Allo)
		{
			CObject.emplace(std::forward<AT>(Ats)...);
		}

		virtual void ReleaseTask() const override {
			CObject.reset();
		}

		virtual void ReleaseSelf() const override {
			auto TAllo = SelfAllocator;
			this->~NormalTaskImplement();
			TAllo.deallocate_object(this, 1);
		}

		virtual void Execute(TaskSystem& System) const override {
			assert(CObject.has_value());
			(*CObject)(System);
		}

		Allocator<NormalTaskImplement<T>> SelfAllocator;
		std::optional<T> CObject;
	};

	template<typename FuncT>
	TaskPtr TaskSystem::CreateLambdaTask(TaskProperty Property, FuncT&& Func, MemoryResource* Resource)
	{
		using PureType = std::remove_reference_t<FuncT>;
		
		Allocator<NormalTaskImplement<PureType>> Allo{Resource};
		auto Ptr = Allo.allocate_object(1);
		new (Ptr) NormalTaskImplement<PureType>{
			Property, Allo, std::forward<FuncT>(Func)
		};
		TaskPtr TPtr{Ptr};
		return TPtr;
	}
	*/


}