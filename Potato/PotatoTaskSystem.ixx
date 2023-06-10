export module Potato.TaskSystem;
export import Potato.STD;
export import Potato.Misc;

export namespace Potato::Task
{
	struct TaskSystem
	{
		struct ExecuteInterface
		{
			
		};

		struct Task
		{
			
		};

		struct SubTask
		{
			
		};

		TaskSystem(std::size_t ThreadCount = std::thread::hardware_concurrency() - 1);
		~TaskSystem();

		void AddRef();
		void SubRef();

	protected:

		void Executor();

		std::atomic_bool Available = true;
		Misc::AtomicRefCount Ref;
		std::vector<std::thread> Thread;
	};
}