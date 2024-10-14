# PotatoIR

动态类型库，支持动态拼接数据类型，并提供动态类型的基础反射

	```cpp
	import PotatoIR;
	using namespace Potato::IR;
	```

以基础类型为组件，动态构建新的数据类型，并且新的数据类型在内存上连续分布。

	```cpp
	// 我们所要构建的目标数据
	struct K
	{
		std::size_t k = 0;
		float I = 0.0f;
		std::size_t o[2] = {1, 2};
	};

	// 默认初始化的值
	std::size_t u = 100;
	float icc = 100.0f;
	std::size_t kl[2] = {288, 899};

	// 根据类型构建一个反射信息
	StructLayout::Member me[] = 
	{
		{
			// 获取静态元数据
			StaticAtomicStructLayout<std::size_t>::Create(),
			// 成员命名
			u8"k",
			// 数组大小
			1,
			// 默认初始值
			&u
		},
		{
			StaticAtomicStructLayout<float>::Create(),
			u8"I",
			1,
			&icc
		},
		{
			StaticAtomicStructLayout<std::size_t>::Create(),
			u8"o",
			2,
			kl
		},
	};

	// 创建一个动态数据类型
	auto P = DynamicStructLayout::Create(u8"K", me);

	// 要修改的值
	K i;

	auto span = P->GetMemberView();

	// 从反射中获取第一个成员的起始指针
	auto ref1 = P->GetDataAs<std::size_t>(span[0], &i);
	*ref1 = 10086;

	// 第一个值已被修改
	assert(i.k == 10086);

	// 依据动态数据类型，调用复制构造函数构造一个实例
	auto obj = StructLayoutObject::DefaultConstruct(P, 1);
	
	// 获取首地址
	K* obj2 = static_cast<K*>(obj->GetData());
	```

