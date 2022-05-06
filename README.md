[TOC]

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

## 2. PotatoMisc

一些杂项，放置一些不便分类的功能类（命名空间namespace Potato::Misc）

1. IndexSpan 类似于std::span，但IndexSpan只储存了两个std::size_t（可定制），用于表示连续内存中的一段。
2. AtomicRefCount 支持原子操作的引用计数器，主要用于智能指针。
3. AlignedSize 用于计算对齐
4. namespace::SerilizerHelper 相关功能，用于执行序列化

## 3. PotatoStrEncode

字符转换，用以处理char8_t，char16_t，char32_t和wchar_t之间的转化（命名空间namespace Potato::StrEncode）

1. CoreEncoder 对于单个字符的转化

	```cpp
	std::u16string_view Str = u"1234";
	// 对Str字符串进行检测，并统计Str内的第一个字符进行统计，返回Str的第一个字符占用的数组空间和转换所需要的字符空间。
	// 对Str内的有效性不做要求
	EncodeInfo Info = CharEncoder<char16_t, char8_t>::RequireSpaceOnce(Str); 

	// 同RequireSpaceOnce ，但要求Str不为空且内部字符均有效，效率一般情况下比RequireSpaceOnce要高。
	EncodeInfo Info2 = CharEncoder<char16_t, char8_t>::RequireSpaceOnceUnSafe(Str); 

	std::u8string Target;
	// 预先分配空间
	Target.resize(Info2.TargetSpace);

	// 进行转换，其内部依赖 RequireSpaceOnceUnSafe ，所以必须要求保证Str非空且均有效。
	EncodeInfo Info3 = CharEncoder<char16_t, char8_t>::EncodeOnceUnSafe(Str, Target);

	// 切换到第二个字符
	Str = Str.substr(Info2.SourceSpace);
	```

2. StrCodeEncoder 对于字符串的转化

	```cpp
	std::u16string_view Str = u"123456\r\n12345";
	// 对Str字符串进行检测，并统计Str内的所有有效字符串，返回Str的有效字符串占用的空间和转换所需要的空间。
	// 对Str内的有效性不做要求
	EncodeInfo Info = StrEncoder<char16_t, char8_t>::RequireSpaceOnce(Str); 

	// 同RequireSpaceOnce ，但要求Str不为空且内部字符均有效，效率一般情况下比RequireSpaceOnce要高。
	EncodeInfo Info2 = StrEncoder<char16_t, char8_t>::RequireSpaceOnceUnSafe(Str); 

	std::u8string Target;
	// 预先分配空间
	Target.resize(Info2.TargetSpace);

	// 进行转换，其内部依赖 RequireSpaceOnceUnSafe ，所以必须要求保证Str非空且均有效。
	EncodeInfo Info3 = StrEncoder<char16_t, char8_t>::EncodeOnceUnSafe(Str, Target);

	// 同RequireSpaceOnceUnSafe，但判断到\r\n或者\n这种字符组合，会自动停止。
	// 第二个参数决定Info4的CharacterCount和TargetSpace是否包含换行符。SourceSpace必定包含换行符。
	EncodeInfo Info4 = StrEncoder<char16_t, char8_t>::RequireSpaceLineUnsafe(Str, true);

	std::u8string LineTarget;
	// 预先分配空间
	LineTarget.resize(Info4.TargetSpace);
	
	// 根据有效字符数进行转换
	EncodeInfo Info5 = StrEncoder<char16_t, char8_t>::EncodeOnceUnSafe(Str, LineTarget, Info4.CharacterCount);

	```

3. DocumentReader / DocumenetReaderWrapper 纯字符串文档阅读器（目前只支持utf8和utf8 with bom两种文本编码格式）

	```cpp
	DocumentReader Reader(L"C:/Sample.txt");
	if(Reader)
	{
		// 成功打开
		std::vector<std::byte> Tembuffer;
		// 直接分配空间储存所有二进制数据
		Tembuffer.resize(Reader.RecalculateLastSize());
		// 根据缓冲区创建Wrapper，注意，此时Wrapper关联了Tembuffer，所有Tembuffer的扩缩容或者其他改变储存地址的操作都会使Wrapper的操作崩溃。
		DocumenetReaderWrapper Wrapper = Reader.CreateWrapper(Tembuffer);
		// Reader 将数据写入 Wrapper 中。注意，写入的数据量与 Wrapper 关联的内存大小和内容有直接关系。
		// 该操作不保证会将所有数据写入，除非Wrapper能保证其内部的缓存空间大于或等于文件内的所有数据，并且Wrapper内部的指针位于开头。
		FlushResult FR = Reader.Flush(Wrapper);

		while(Wrapper)
		{
			std::u32string ReadBuffer;
			// 消耗Wrapper内的数据，直到遇上换行符。
			// 并将数据转成char32_t的格式写入ReadBuffer中，该操作只会往ReadBuffer的末尾添加数据。
			Wrapper.ReadLine(ReadBuffer);
			// ...
		}
	}
	```

4. DocumentWriter 纯字符串文档写入器（目前支持写入utf8和utf8 with bom两种文本编码格式）

	```cpp
	// 重新打开文件，写入UTF8的BOM，若文件已存在，则覆盖
	DocumentWriter Writer(L"C:/Sample.txt", DocumenetBomT::UTF8);
	std::u32string_view Str = U"12344";
	// 将字符串转换成 DocumenetBomT 所对应的格式，写入文件的末尾。
	Writer.Write(Str);
	```





