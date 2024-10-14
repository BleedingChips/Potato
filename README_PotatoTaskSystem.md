# PotatoTaskSystem

基于线程池的多线程任务系统

	```cpp
	import PotatoTaskSystem;
	using namespace Potato::Task;
	```

`TaskSyetem`内部维护了一个基于`std::thread`的内存池，并维护了一个单一任务队列，每一个线程都被赋予了特定的属性和权限，以控制其从任务队列获取任务的能力。

一般流程：

1. 定义创建一个实例，并且拉起对应的线程：

	```cpp
	TaskContext conotext;
	context.AddGroupThread({}, TaskContext::GetSuggestThreadCount());
	```

	所有往这个`Context`提交的任务和内部维护的线程池，在Context销毁后都会销毁。

2. 创建`Task`

	```cpp
	std::size_t Count = 0;
	// 从 lambda 创建一个任务
	Task::Ptr Lambda = Task::CreateLambdaTask([&Count](Potato::Task::TaskContextWrapper& Status, Potato::Task::Task& This){
		std::println("Count :{0} {1} {2}", Count, static_cast<std::size_t>(Status.status), Status.thread_id);
		Count++;
		if(Count <= 20)
			// 只循环提交20次，20次之后就不再循环
			Status.context.CommitDelayTask(&This, std::chrono::milliseconds{50}, Status.task_property);
	});

	// 创建自定义的任务
	struct UserTask : public Task
	{
		// 需要自己提供引用计数的功能
		virtual void AddTaskRef() const;
		virtual void SubTaskRef() const;

		// 处理任务的执行
		virtual void TaskExecute(TaskContextWrapper& status);

		// 处理任务的中断（如TaskContext）时的回调
		virtual void TaskTerminal(TaskProperty property) noexcept {};
		
	};
	```

3. 提交

	```cpp
	// 标志该任务的优先级与可达性
	TaskProperty tp;

	// GLOBAL_TASK 指任意线程均可执行，也可以指定某个Group的线程池或者某个特定线程
	tp.filter.category = Category::GLOBAL_TASK;
	
	// 提交并立即执行
	context.CommitTask(Lambda, tp);
	
	// 延时提交
	context.CommitDelayTask(Lambda, std::chrono::milliseconds{ 50 }, tp);
	```

4. 堵塞并尝试运行Task，直到所有任务被执行完毕

	```cpp
	context.ProcessTaskUntillNoExitsTask({});
	```

