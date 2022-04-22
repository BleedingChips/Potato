# Potato

个人库，用来管理和存储一些常用的功能，其包含以下内容：

## 1. PotatoTMP 

模板元编程库（命名空间 Potato::TMP）

其包含如下功能：

1. IsOneOf 用来判断某个类型是否在类型集中。
    ```cpp
    IsOneOf<A, A, B, C>::Value; // true， A在{A,B,C} 中
    IsOneOf<D, A, B, C>::Value; // false，D不在{A,B,C} 中
    ```
2. IsNotOneOf IsOneOf的否。
3. IsRepeat 类型中是否有重复的类型。

	```cpp
	IsRepeat<D, A, A, C>::Value; // true No.1 A And No.2 A
	IsRepeat<D, A, B, C>::Value; // false
	```

4. ItSelf 占位符
5. Instant 类型合并功能

	```cpp
	Instance<std::tuple, A, B, C>:: template Append<D, E>; // Instance<std::tuple, A, B, C, D, E>

	Instance<std::tuple, A, B, C>:: template AppendT<D, E>; // std::tuple<A, B, C, D, E>

	Instance<std::tuple, A, B, C>:: template Front<D, E>; // Instance<std::tuple, D, E, A, B, C>

	Instance<std::tuple, A, B, C>:: template FrontT<D, E>; // std::tuple<D, E, A, B, C>
	```

6. TypeTuple std::tuple 类似物，但不会分配空间
7. Replace 类型替换功能

	```cpp
	Replace<std::tuple<A, B, C>>:: template With<TypeTuple>; // TypeTuple<A, B, C>
	```

8. Exist 存在判断，可以用来检测某个函数或者变量是否存在

	```cpp
	struct A { void Function(); };
	struct B{};

	template<typename Type, typename = std::enable_if_t<decltype(Type::Function)>>
	struct Role {};

	Exist<A, Role>::Value; // true
	Exist<B, Role>::Value; // false
	```

9. IsFunctionObjectRole 判断某个Object是不是CallableObject

	```cpp
	auto F = [](){};
	IsFunctionObjectRole<decltype(F)>::Value; // true
	```

10. FunctionInfo 判断函数的类型

	```cpp
	void Function(int32_t, ...) noexcept;
	using F = FunctionInfo<decltype(Function)>;
	F::IsConst; // false, const
	F::IsVolatile; // false volatile
	F::IsRef; // false, &
	F::IsMoveRef; // false, &&
	F::IsNoException; // true noexcept
	F::IsEllipsos; // true , ...
	F::ReturnType; // void
	F::template PackParameters<TypeTuple>; // TypeTuple<int32_t>
	F::template PackReturnParameters<TypeTuple>; // TypeTuple<void, int32_t>
	```

11. ConstString 字符串常量类型

	```cpp
	ConstStringHolder<"sadasd">;
	```