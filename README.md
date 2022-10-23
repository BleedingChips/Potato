# Potato

个人库，用来管理和存储一些常用的功能，基于`MSVC`下的CPP最新标准。

本库包含以下主要功能：

1. [PotatoTMP](#PotatoTMP)
	
	模板元编程功能库。

2. [PotatoIntrusivePointer](#PotatoIntrusivePointer)
	
	线程安全的嵌入式智能指针功能库。
3. [PotatoStrEncode](#PotatoStrEncode)	
	
	`Unicode`之间的编码相互转换的功能库。
4. [PotatoSLRX](#PotatoSLRX)
	
	基于柔性`LR(x)`的语法分析库。
5. [PotatoReg](#PotatoReg)
	
	为词法分析特化实现的，基于DFA的正则表达式分析库。
6. [PotatoStrFormat](#PotatoStrFormat)
	
	`std::format`类似物，提供数据与字符串之间的相互转化。
7. [PotatoEnbf](#PotatoEnbf)
	
	`ENBF`词法/语法分析库。

计划实现的功能：

1. PotatoIR                 类型元数据，符号表，中间语言，一个可执行的最小系统。
2. PotatoVirtualPath        虚拟路径系统。

## 目录包含

1. `Potato/Public` 所有的头文件。
2. `Potato/Private` 所有的实现文件。
3. `Potato/Project/Potato` VisualStudio2022的静态库工程文件，可以直接将其加入到解决方案列表下的项目中。
4. `Potato/Project/UnitTesting` VisualStudio2022的单元测试的解决方案文件。

## 依赖非标准的第三方库

无

## 使用方式

将`Potato/Public`和`Potato/Private`包含到程序中即可。

## PotatoTMP 

模板元编程库（namespace Potato::TMP）

* IsOneOf 

	用来判断某个类型是否在后续的类型集中。

	```cpp
	static_assert(IsOneOf<A, A, B, C>::Value); // true
	static_assert(IsOneOf<A, D, B, C>::Value); // false
	```

* IsNotOneOf 

	IsOneOf的否。
* IsRepeat 

	类型中是否有重复的类型。

	```cpp
	static_assert(IsOneOf<A, A, B, C>::Value); // true
	static_assert(IsOneOf<A, D, B, C>::Value); // false
	```

* Instant 
	
	延迟构造类型。

	```CPP
	template<typename ...A> class TT;
	using T1 = Instant<TT, A>;
	static_assert(std::is_same_v<T1::template Append<B>, TT<A, B>>); // true
	static_assert(std::is_same_v<T1::template Front<B>, TT<B, A>>);// true
	```

* Replace 

	类型替换。
	```cpp
	template<typename ...A> class TT;
	template<typename ...A> class TT2;
	static_assert(std::is_same_v<Replace<TT<A, B>::template With<TT2>, TT2<A, B>>); // true
	```
* Exist
	
	存在判断，用来检测某个函数或者变量是否存在，SFINAE。
	```cpp
	template<typename Type, typename = std::void_t<decltype(&Type::Value1)>> struct ExistValue1Role {};

	struct Sample1 { int32_t Value1; };

	static_assert(Exist<Sample1, ExistValue1Role>::Value); // true
	```

* FunctionInfo 

	函数类型签名的萃取。
	```cpp
	struct Sample1
	{
		int32_t Function(char32_t, ...) && noexcept;
	};

	using Info = FunctionInfo<decltype(&Sample1::Function)>;

	static_assert(!Info::IsConst);// true
	static_assert(Info::IsEllipsis);// true
	static_assert(Info::IsMoveRef);// true
	static_assert(Info::IsNoException);// true
	static_assert(!Info::IsRef);// true
	static_assert(!Info::IsVolatile);// true

	template<typename ...A> class TT;

	static_assert(std::is_same_v<Info::OwnerType, Sample1>); // true
	static_assert(std::is_same_v<Info::ReturnType, int32_t>); // true
	static_assert(std::is_same_v<Info::PackParameters<TT>, TT<char32_t>>); // true
	static_assert(std::is_same_v<Info::PackReturnParameters<TT>, TT<int32_t, char32_t>>); // true
	```
* TempString 

	字符串常量类型，可用作模板参数。

	```cpp
	static_assert(std::is_same_v<CST<u"1234">, CST<u"1234">>,
	```

## PotatoIntrusivePointer

嵌入式智能指针。（namespace Potato）

```cpp
struct S{ void AddRef(); void SubRef(); }
struct Type2Wrapper
{
	static void AddRef(S* Type) { Type->AddRef(); }
	static void SubRef(S* Type) { Type->SubRef(); }
};
IntrusivePointer<S, Type2Wrapper> Ptr = new S{};
```

## PotatoStrEncode

字符转换，用以处理char8_t，char16_t，char32_t和wchar_t之间的转化（namespace Potato::StrEncode）

* CharEncoder 
	
	对于单个字符的转化
	```cpp
	std::u8string_view Str;
	EncodeInfo Info = StrEncode<char8_t, char16_t>::RequireSpaceOnce(Str);
	std::u16string Str2;
	Str2.resize(Info.RequireSpace);
	StrEncode<char8_t, char16_t>::EncodeOnceUnSafe(Str, Str2);
	```

* StrEncoder 

	对于字符串的转化
	```cpp
	std::u8string_view Str;
	EncodeInfo I1 = StrEncoder<char8_t, char16_t>::RequireSpace(Str);
	std::u16string Str2;
	R1.resize(I1.TargetSpace);
	StrEncoder<ST, TT>::EncodeUnSafe(Str, Str2);
	```

## PotatoSLRX

基于LR(X)的语法分析器。（namespace Potato::SLRX）

该分析器的使用步骤为：

1. 创建分析表
2. 序列化分析表（可选）
3. 创建一个分析器
4. 使用分析器分析终结符系列，等到AST的构建步骤。
5. 根据构建步骤，直接生成AST或者直接生成目标语言。

* 创建一个分析表

	分析表主要由终结符，非终结符等一系列符号和一部分控制符构成。

	终结符和非终结符为`SLRX::StandardT(uint32_t)`类型的变量，通过：

	```cpp
	Symbol::AsTerminal(static_cast<StandardT>(Value));
	Symbol::AsNoTerminal(static_cast<StandardT>(Value));
	```

	来创建对应的终结符和非终结符。

	一般建议通过枚举值来定义可读性高的符号类型，然后再强转成`Symbol`使用，如：

	```cpp
	enum class Noterminal : StandardT
	{
		Exp = 0,
		Exp1 = 1,
		Exp2 = 2,
	};

	constexpr Symbol operator*(Noterminal input) { return Symbol::AsNoTerminal(static_cast<StandardT>(input)); }

	enum class Terminal : StandardT
	{
		Num = 0,
		Add,
		Sub,
		Mul,
		Dev,
		LeftBracket,
		RigheBracket
	};

	constexpr Symbol operator*(Terminal input) { return Symbol::AsTerminal(static_cast<StandardT>(input)); }
	```

	分析表主要包含四个部分
		
	1. StartSymbol 

		开始符号，见`LR`的概念。
			
	2. Production 

		产生式。一个产生式如下：

		```cpp
		ProductionBuilder Builder{NoTerminlSymbol, {Symbols...}, Mask, Predict};
		```

		* Noterminal 表示该产生式的左部。是一个非终结符。
		* Symbols... 表示该产生式的右部。

			产生式的右部可以是终结符，非终结符和跟随在非终结符后的标记值。标记值用来禁止产生式的规约路径，比如：

			```cpp
			std::vector<ProductionBuilder> Lists = {
				{*Exp, { *Exp, *Exp, 2 }, 2},
				{*Exp, {*Num}},
			};
			```

			这里的`2`表示其前方的`*Exp`无法由标记为`2`的产生式规约产生。若无该标记，对于上述的产生式以及终结符序列{`*Num`, `*Num`, `*Num`}，有两种等效的规约路径：
				
			```
			Num,Num,Num -(Exp:Num)-> Exp<Num>, Exp<Num>, Exp<Num> -(Exp:Exp Exp)-> Exp<Num> Exp<Exp, Exp> -(Exp:Exp Exp)-> Exp<Exp, Exp>

			Num,Num,Num -(Exp:Num)-> Exp<Num>, Exp<Num>, Exp<Num> -(Exp:Exp Exp)-> Exp<Exp, Exp> Exp<Num> -(Exp:Exp Exp)-> Exp<Exp, Exp>
			```

			加了标记值后，该规约步骤会被禁止：

			```
			Exp<Num>, Exp<Num>, Exp<Num> -(Exp:Exp Exp)-> Exp<Num> Exp<Exp, Exp>
			```

			所以最终将只有一个规约路径。

		* Mask 该产生式的标记值，用来在后续的语法制导中使用，默认为0。

		* Predict 该产生是否需要生成前置标记符。

			对于产生式`Exp:Num Num`和终结符序列{`*Num`, `*Num`}，将会产生如下的路径：{`Shift` `Shift` `Reduce<Exp>`}。

			若标记生成前置标记符，则产生的路径如下：{`Predice<Exp>` `Shift` `Shift` `Reduce<Exp>`}。这里会影响后续的语法制导。

			由于前置标记需要在产生Reduce行为后，在路劲的中间插入一个新步骤，所以会对性能有负面影响。

	3. OpePriority

		定义符号优先级。

		其定义方式如：

		```cpp
		std::vector<OpePriority> OP = 
		{
			{{*Terminal::Mul}, Associativity::Left}, 
			{{*Terminal::Add， *Terminal::Sub}, Associativity::Right},
		}
		```

		这里表示`Mul`的优先级大于`Add`和`Sub`，并且为左结合，`Add`和`Sub`的优先级相等小于`Mul`，并且为右结合。

	4. MaxForwardDetect

		标记该产生式最多支持的`LR(x)`。由于判断一个语法是否有歧义是一个`NP`问题，但判断一个语法能否被`LR(x)`解析是一个`P`问题。所以这里可以设置最高的判断数量，超过该数量将会抛出异常。另，太高的数字会严重影响生成的性能。默认值为3。

	对于一个解析四则运算的产生式，可以有下面的分析表：

	```cpp
	LRX Table(
		*Noterminal::Exp,
		{
			{*Noterminal::Exp, {*Terminal::Num}, 1},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Add, *Noterminal::Exp}, 2},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3, true},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Sub, *Noterminal::Exp}, 4},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Dev, *Noterminal::Exp}, 5},
		},
		{
			{{*Terminal::Multi, *Terminal::Dev}},
			{{*Terminal::Add, *Terminal::Sub}, Associativity::Left},
		}
	);
	```

	也可以进行序列化后直接使用：

	```cpp
	std::vector<StandardT> Buffer = TableWrapper::Create(Tab);
	```

	若创建失败，则会抛出以`Potato::SLRX::Exception::Interface`为基类的异常。

* 使用分析表对终结符进行解析：

	```cpp
	
	```



<!-- SLRX是一款“柔性”的语法分析器，对于用户给定的X（默认为3），可以自动识别LR(X)的语法，并生成对应的分析表。

在此基础上，通过特定的储存方式，使得表本身有着更小的体积。

对于一些带有歧义的语法，也会在生成表格的时候，通过异常的方式抛出给用户，方便用户进行修改。

在性能上，当X为0时有着最小的体积与最高的效率。对于越大的X，其在生成表格和在处理语法时，性能越差。在最坏情况下，其性能复杂度为指数级。

为了方便构造分析表，其提供了一些功能，使得用户可以自定义运算符优先级（类似+-*/），以及防止深度为1的Reduce路径。

目前该分析器是单线程，不支持错误回退，并且是非异常安全的。

一般流程:

1. 将终结符与非终结符转换成内部专用的Symbol。
2. 使用Symbol，设置开始符，产生式，运算符优先级，构建分析表，并序列化成可读的Table。
3. 使用Table，对目标的Symbol序列进行分析，产生Step序列，Step序列的信息足够产生一颗AST树。
4. 对Step序列执行分析，产生目标语言。

该模块遇到错误会抛出异常，异常一般以`Potato::SLRX::Exception::Interface`为基类。

## 5. PotatoReg

基于`char32_t`的正则表达式库，依赖于`PotatoDLr`。（namespace Potato::Reg）

目前只支持以下几种元字符：

1. `.` `-`
2. `*` `*?`
3. `+` `+?`
4. `?` `??`
5. `|` `()` `[]` `[^]` `(?:)`
6. `{x,y}` `{X}`  `{X,}` `{,Y}` `{x,y}?` `{X}?`  `{X,}?` `{,Y}?`

由于一些优化，类似于`(.*)*`这种正则表达式，会抛出异常，其特征为：

>对于一个非元字符，在预查找下一个需要的非元字符时，只能经过有限次的不消耗非元字符的查找。

支持多个正则合并到一个正则上，合并的时候会有一次优化，会一同进行匹配。

在匹配时，支持以下三种操作：

1. Search 只需要字符串其中的一段满足正则既返回真。
2. March 需要字符串全段满足正则才返回。
3. FrontMarch 需要字符串的开头一段满足才返回。
4. GreedyFrontMatch 同 FrontMarch ，但会尝试匹配最长的匹配项。

一般的，GreedyFrontMatch 用在词法分析上。

该模块遇到错误时会抛出异常，异常一般以`Potato::Reg::Exception::Interface`为基类。

## 6. PotatoEbnf

依赖于`PotatoSLRX`与`PotatoReg`的`Ebnf`范式。（namespace Potato::Ebnf）

一般而言，除了支持从字符串构建语法分析表，并且支持`|` `()` `[]` `{}`等Ebnf范式的操作符，以及支持内置的一个词法分析器外，其余和 SLRX 无差别。

## 7. PotatoPath

文件路径相关库，施工中。

## 8. PotatoIR

编译器中间表示相关功能类，施工中。

## 9. PotatoInterval

用以表达数学上的集合概念的库，施工中。

-->













