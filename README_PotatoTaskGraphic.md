# PotatoTaskFlow

```cpp
import PotatoTaskFlow;
using namespace Potato::Task;
```

基于`PotatoTaskSystem`与有向无环图的多线程安全任务图系统，通过改良的拓扑算法，在添加新的有向边时能够快速判断是否存在有向边。

使用流程：

1. 创建`TaskContext`:

	```cpp
	TaskContext context;
	context.AddGroupThread({}, 5);
	```

2. 构建`TaskFlow`并添加对应的任务：

	```cpp
	struct DefaultTaskFlow : TaskFlow
	{
		void AddTaskFlowRef() const override {}
		void SubTaskFlowRef() const override {}
		void TaskFlowExecuteBegin(TaskFlowContext& context) override {}

		void TaskFlowExecuteEnd(TaskFlowContext& context) override
		{}
	};

	// 子图标
	DefaultTaskFlow tf2;

	// 父图标
	DefaultTaskFlow tf;

	auto lambda = [](Potato::Task::TaskFlowContext& status){
		//执行代码
	};

	auto lambda2 = [&](Potato::Task::TaskFlowContext& context)
	{
		//执行代码
	};

	// 添加到图标里
	auto a1 = tf.AddLambda(lambda, {u8"A1"});
	auto a2 = tf.AddLambda(lambda, {u8"A2"});
	auto a3 = tf.AddLambda(lambda, {u8"A3"});
	auto a4 = tf.AddLambda(lambda, {u8"A4"});
	auto a5 = tf.AddLambda(lambda, {u8"A5"});
	// 添加子图
	auto a6 = tf.AddNode(tf2, {u8"SubTask"});
	auto a7 = tf.AddLambda(lambda2, {u8"temporary"});

	// 添加依赖关系，a2必须要在a1执行之后执行
	bool l12 = tf.AddDirectEdge(*a1, *a2);
	bool l23 = tf.AddDirectEdge(*a2, *a3);
	bool l34 = tf.AddDirectEdge(*a3, *a4);

	// 成环了，所以该依赖关系添加失败
	bool l41 = tf.AddDirectEdge(*a4, *a1);

	// 添加互斥关系，a3与a5不能同时执行
	bool m35 = tf.AddMutexEdge(*a3, *a5);

	// 删除依赖关系
	bool r41 = tf.RemoveDirectEdge(*a2, *a3);

	// 无环，可以添加成功
	bool l41_2 = tf.AddDirectEdge(*a4, *a1);
	bool l61 = tf.AddDirectEdge(tf2, *a1);
	bool l46 = tf.AddDirectEdge(*a4, tf2);
	```

3. 更新并提交

	```cpp
	tf.Update();
	tf.Commited(context, {u8"first Task Flow"});
	```

4. 等待执行完毕

	```cpp
	context.ProcessTaskUntillNoExitsTask({});
	```


