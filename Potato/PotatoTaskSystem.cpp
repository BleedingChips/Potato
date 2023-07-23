module;

#include <cassert>

module PotatoTaskSystem;


namespace Potato::Task
{
	
	/*
	struct TaskSystemImp : public TaskSystemReference
	{
		using TaskSystemReference::TaskSystemReference;

		std::optional<TaskSystem> System;
	};

	void TaskSystemReference::AddStrongRef(TaskSystem* Ptr)
	{
		SRef.AddRef();
	}
	
	void TaskSystemReference::SubStrongRef(TaskSystem* Ptr) {
		if (SRef.SubRef())
		{
			static_cast<TaskSystemImp*>(this)->System.reset();
		}
	}

	void TaskSystemReference::AddWeakRef(TaskSystem* Ptr) {
		WRef.AddRef();
	}

	void TaskSystemReference::SubWeakRef(TaskSystem* Ptr) {
		if (WRef.SubRef())
		{
			auto OldMemoryResource = RefMemoryResource;
			static_cast<TaskSystemImp*>(this)->~TaskSystemImp();
			OldMemoryResource->deallocate(this, 
				sizeof(TaskSystemImp),
				alignof(TaskSystemImp)
			);
		}
	}

	bool TaskSystemReference::TryAddStrongRef(TaskSystem* Ptr) {
		return SRef.TryAddRefNotFromZero();
	}
	*/
}