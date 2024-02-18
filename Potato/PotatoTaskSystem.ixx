module;

#include <cassert>

export module PotatoTaskSystem;

import std;
import PotatoMisc;
import PotatoPointer;
import PotatoIR;


export namespace Potato::Task
{

	export struct TaskContext;

	enum class Priority : std::size_t
	{
		VeryHigh = 64,
		High = 32,
		Normal = 16,
		Low = 4,
		VeryLow = 1,
	};


	enum class Category : std::size_t
	{
		GLOBAL_TASK,
		GROUP_TASK,
		THREAD_TASK
	};

	struct AppendData
	{
		std::array<std::size_t, 2> datas;
		std::size_t& operator[](std::size_t index) { return datas[index]; }
	};


	struct TaskProperty
	{
		Priority priority = Priority::Normal;
		std::u8string_view name = u8"Unnamed Task";
		Category category = Category::GLOBAL_TASK;
		std::size_t group_id = 0;
		std::thread::id thread_id;
	};

	enum class Status : std::size_t
	{
		Normal,
		Close
	};

	enum class ThreadAcceptable : std::size_t
	{
		UnAccept,
		SpecialAccept,
		AcceptAll
	};

	struct ThreadProperty
	{
		std::size_t group_id = 0;
		ThreadAcceptable global_accept = ThreadAcceptable::AcceptAll;
		ThreadAcceptable group_accept = ThreadAcceptable::SpecialAccept;
		std::u8string_view name;
	};

	struct ExecuteStatus
	{
		Status status = Status::Normal;
		TaskContext& context;
		TaskProperty task_property;
		std::thread::id thread_id;
		ThreadProperty thread_property;
		AppendData user_data;
	};

	

	struct Task
	{

		struct Wrapper
		{
			void AddRef(Task const* ptr) { ptr->AddTaskRef(); }
			void SubRef(Task const* ptr) { ptr->SubTaskRef(); }
		};

		using Ptr = Pointer::IntrusivePtr<Task, Wrapper>;

		template<typename FunT>
		static Ptr CreateLambdaTask(FunT&& func, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
			requires(std::is_invocable_v<FunT, ExecuteStatus& , Task::Ptr>)
		;

	protected:
		
		virtual void AddTaskRef() const = 0;
		virtual void SubTaskRef() const = 0;

		virtual void TaskExecute(ExecuteStatus& status) = 0;
		virtual void TaskTerminal(TaskProperty property) noexcept {};
		virtual ~Task() = default;

		friend struct TaskContext;
		friend struct PointerWrapper;
	};

	export struct TaskContext
	{

		static std::size_t GetSuggestThreadCount();

		bool AddGroupThread(ThreadProperty property, std::size_t thread_count = 1);
		std::size_t GetRandomThreadID(std::size_t group_id);
		std::size_t GetRandomThreadID();

		void RequestCloseThread(std::size_t thread_id);
		void RequestCloseGroupThread(std::size_t thread_id);
		void RequestCloseAllThread();

		struct ContextStatus
		{
			std::size_t exist_task_count = 0;
			bool executed = false;
			bool has_acceptable_task = false;
			std::chrono::steady_clock::time_point next_waiting_task;
		};

		template<typename Func>
		std::size_t ProcessTask(ThreadProperty property, Func&& stop_condition, std::chrono::steady_clock::duration sleep_time = std::chrono::microseconds{10})
			requires(std::is_invocable_r_v<bool, Func, ContextStatus>)
		{
			std::size_t count = 0;
			auto id = std::this_thread::get_id();
			while(true)
			{
				auto re = ProcessTaskOnce(property, id, std::chrono::steady_clock::now());
				if(stop_condition(re))
				{
					if(!re.executed)
					{
						std::this_thread::sleep_for(sleep_time);
					}else
					{
						count += 1;
						std::this_thread::yield();
					}
				}else
				{
					return count;
				}
			}
		}

		std::size_t ProcessTaskUntillNoExitsTask(ThreadProperty property, std::chrono::steady_clock::duration sleep_time = std::chrono::microseconds{ 10 })
		{
			return ProcessTask(property, [](ContextStatus status)->bool{ return status.exist_task_count != 0; }, sleep_time);
		}

		std::size_t CloseAllThread();
		std::size_t CloseAllThreadAndWait();

		bool CommitTask(Task::Ptr task, TaskProperty property = {}, AppendData data = {});
		bool CommitDelayTask(Task::Ptr task, std::chrono::steady_clock::time_point time_point, TaskProperty property = {}, AppendData data = {});
		bool CommitDelayTask(Task::Ptr task, std::chrono::steady_clock::duration duration, TaskProperty property = {}, AppendData data = {})
		{
			return CommitDelayTask(std::move(task), std::chrono::steady_clock::now() + duration, property, data);
		}


		TaskContext(std::pmr::memory_resource* resource = std::pmr::get_default_resource());
		virtual ~TaskContext();

	protected:

	private:

		struct TaskTuple
		{
			TaskProperty property;
			Task::Ptr task;
			AppendData data;
		};

		static bool Accept(TaskProperty const& property, ThreadProperty const& thread_property, std::thread::id thread_id);
		std::tuple<std::optional<TaskTuple>, std::chrono::steady_clock::time_point> PopLineUpTask(ThreadProperty property, std::thread::id thread_id, std::chrono::steady_clock::time_point current_time);

		void LineUpThreadExecute(std::stop_token ST, ThreadProperty property, std::thread::id thread_id);

		ContextStatus ProcessTaskOnce(ThreadProperty property, std::thread::id thread_id, std::chrono::steady_clock::time_point current);

		std::shared_mutex thread_mutex;
		
		struct ThreadCore
		{
			ThreadProperty property;
			std::thread::id thread_id;
			std::jthread thread;
		};
		std::pmr::vector<ThreadCore> thread;

		std::mutex execute_thread_mutex;
		std::condition_variable cv;
		std::condition_variable main_cv;
		struct LineUpTuple
		{
			TaskTuple tuple;
			std::size_t priority;
			std::optional<std::chrono::steady_clock::time_point> time_point;
		};
		std::pmr::vector<LineUpTuple> line_up_task;
		std::size_t total_task_count = 0;
		Status status = Status::Normal;
		bool already_add_priority = false;
	};

}

namespace Potato::Task
{

	template<typename TaskImpT>
	struct TaskImp : public Task, public Potato::Pointer::DefaultIntrusiveInterface
	{

		virtual void Release() override
		{
			auto Rec = Record;
			this->~TaskImp();
			Rec.Deallocate();
		}

		template<typename FunT>
		TaskImp(Potato::IR::MemoryResourceRecord Record, FunT&& Fun)
			: Record(Record), TaskInstance(std::forward<FunT>(Fun))
		{
			
		}

		virtual void TaskExecute(ExecuteStatus& Status) override
		{
			Task::Ptr ThisPtr{this};
			TaskInstance.operator()(Status, std::move(ThisPtr));
		}

	protected:

		virtual void AddTaskRef() const override { DefaultIntrusiveInterface::AddRef(); }
		virtual void SubTaskRef() const override { DefaultIntrusiveInterface::SubRef(); }

	private:

		Potato::IR::MemoryResourceRecord Record;
		TaskImpT TaskInstance;
	};

	template<typename FunT>
	Task::Ptr Task::CreateLambdaTask(FunT&& Func, std::pmr::memory_resource* Resource)
		requires(std::is_invocable_v<FunT, ExecuteStatus&, Task::Ptr>)
	{
		using Type = TaskImp<std::remove_cvref_t<FunT>>;
		assert(Resource != nullptr);
		auto Record = Potato::IR::MemoryResourceRecord::Allocate<Type>(Resource);
		if(Record)
		{
			return new (Record.Get()) Type{ Record, std::forward<FunT>(Func)};
		}
		return {};
	}
}