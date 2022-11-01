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

将`Potato/Public`和`Potato/Private`包含到程序中编译即可。

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
			Num Num Num -[Exp:Num]-> Exp<Num> Exp<Num> Exp<Num> -[Exp:Exp Exp]-> Exp<Num> Exp<Exp, Exp> -[Exp:Exp Exp]-> Exp<Exp, Exp>

			Num Num Num -[Exp:Num]-> Exp<Num> Exp<Num> Exp<Num> -[Exp:Exp Exp]-> Exp<Exp, Exp> Exp<Num> -[Exp:Exp Exp]-> Exp<Exp, Exp>
			```

			加了标记值后，该规约步骤会被禁止：

			```
			Exp<Num> Exp<Num> Exp<Num> -[Exp:Exp Exp]-> Exp<Num> Exp<Exp, Exp>
			```

			所以最终将只有一个规约路径。

			```
			Num Num Num -[Exp:Num]-> Exp<Num> Exp<Num> Exp<Num> -[Exp:Exp Exp]-> Exp<Exp, Exp> Exp<Num> -[Exp:Exp Exp]-> Exp<Exp, Exp>
			```

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

		默认位左结合。

		符号优先级只会处理固定格式的产生式，如：

		```cpp
		{E, {E, +, E}, 2},
		{E, {E, *, E}, 3},
		{E2, {E2, +, E2}, 4},
		```
		
		将会自动更改成：

		```cpp
		{E, {E, +, E, 2}, 2},
		{E, {E, 2, *, E, 2, 3}, 3},
		{E2, {E2, +, E2, 4}, 4},
		```

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

* 使用分析表对终结符进行解析，的到解析路径：

	```cpp
	SymbolProcessor Pro(Tab); // 这里是引用，所以必须保持Table的实例的生命周期
	std::span<Symbol const> Symbols;
	std::size_t TokenIndex = 0;
	std::vector<Symbols> SuggestSymbols;
	for(auto Ite : Symbols)
	{
		if(!Pro.Consume(Ite, TokenIndex++, SuggestSymbols))
			return false; // 不接受该Symbol，接受的Symbols见 SuggestSymbols。
	}

	if(!Pro.EndOfFile(TokenIndex++, SuggestSymbols))
		return false; // 不接受该Symbol，接受的Symbols见 SuggestSymbols。
	
	std::span<ParsingStep const> = Pro.GetSteps();
	```

* 根据解析路径，生成AST树或直接生成结果。

	```cpp
	auto Func = [&](VariantElement Ele) -> std::any {
		if (Ele.IsNoTerminal())
		{
			NTElement NE = Ele.AsNoTerminal();
			switch (NE.Reduce.Mask)
			{
			case 1:
				return NE[0].Consume();
			case 2:
				return NE[0].Consume<int32_t>() + NE[2].Consume<int32_t>();
			case 3:
				return NE[0].Consume<int32_t>() * NE[2].Consume<int32_t>();
			case 4:
				return NE[0].Consume<int32_t>() - NE[2].Consume<int32_t>();
			case 5:
				return NE[0].Consume<int32_t>() / NE[2].Consume<int32_t>();
			default:
				break;
			}
			return {};
		}
		else if(Ele.IsTerminal()) {
			TElement TE = Ele.AsTerminal();
			return Wtf[TE.Shift.TokenIndex].Value;
		}
		else if(Ele.IsPredict())
		{
			PredictElement PE = Ele.AsPredict();
			return {};
		}
	};

	auto Result = ProcessParsingStepWithOutputType<int32_t>(Pro.GetSteps(), Func);
	```
## PotatoReg

为词法分析器特化的，基于DFA的，可单次同时匹配多条正则表达式的正则库。

* 创建正则表达式：

	```cpp
	DFA Table(u8R"(([0-9]+))", false, Accept{ 2, 3 }); 
	// 序列化版本
	std::vector<Reg::StandardT> Table2 = TableWrapper::Create(Table); 
	```
	false 表示这个字符串是个正则字符串，Accept 表示当该正则表达式匹配成功后所返回的标记值。

	支持创建多个正则表达式：

	```cpp
	MulityRegCreater Crerator;
	Crerator.LowPriorityLink(u8R"(while)", false, {1, 0}); // 1
	Crerator.LowPriorityLink(u8R"(if)", false, {2, 0}); // 2
	Crerator.LowPriorityLink(u8R"([a-zA-Z]+)", false, {3, 0}); // 3
	DFA Table = Crerator.GenerateDFA();
	```
	同时处理多个正则表达式，先创建的优先级高。当多个正则均能同时匹配时，选取优先级最高的。


	由于一些优化，类似于`(.*)*`这种正则表达式，会抛出异常，其特征为：

	>对于一个非元字符，在预查找下一个需要的非元字符时，只能经过有限次的不消耗非元字符的查找。

* 对字符串进行检测：

	目前提供的匹配方式只有整串匹配和头部匹配两种。

	* 整串匹配
  
  		指字符串要与正则表达式完全匹配。若有多个正则表达式，则只需要匹配其中一种即可。

		```cpp
		std::u8string_view Source = u8"(1234567)";
		MatchResult<MatchProcessor::Result> Re = Match(Table, Source); // 整串匹配
		if(Re)
		{
			Re->AcceptData; // 用来标记匹配的是哪个字符串
			std::u8string_view Cap 
				= Re->GetCaptureWrapper().GetTopSubCapture().GetCapture().Slice(Source); // 获取捕获组所捕获的东西。
		}
		```

		> 这里的捕获组与普通的正则有所不同。在普通的正则表达式中，若有正则表达式`(abc)+`，那么该表达式只存在捕获组一个捕获，并且只会捕获最后一个`abc`。但在这里会产生匹配次数个捕获组。

	* 头部匹配

		只需要字符串的前部能匹配一个或多个正则即可。这里有两种匹配方式，既贪婪（匹配最长）和非贪婪（匹配最短）两种。

		```cpp
		std::u8string_view Source = u8"(1234567abc)";
		MatchResult<HeadMatchProcessor::Result> Re = HeadMatch(Table, Source, false); // 整串匹配, 非贪婪
		if(Re)
		{
			Re->AcceptData; // 应该为{2, 3}
			std::u8string_view Cap 
				= Re->GetCaptureWrapper().GetTopSubCapture().GetCapture().Slice(Source); // 获取捕获组所捕获的东西。
			std::u8string_view Total = Re->MainCapture.Slice(Source); // 头部匹配的字符串
		}

		MatchResult<MatchProcessor::Result> Re = Match(Table, Source); // 整串匹配
		```

	由于性能原因，并不支持查询操作，可以在正则前加入`.*?`配合头部匹配进行操作。

## PotatoEbnf

基于`PotatoSLRX`和`PotatoReg`的Ebnf库。

* 创建一个Ebnf

	```cpp
	std::u8string_view EbnfCode1 =
		u8R"(
	$ := '\s+'
	Num := '[1-9][0-9]*' : [1]

	%%%%

	$ := <Exp> ;

	<Exp> := Num : [1];
		:= <Exp> '+' <Exp> : [2];
		:= <Exp> '*' <Exp> : [3];
		:= <Exp> '/' <Exp> : [4];
		:= <Exp> '-' <Exp> : [5];
		:= '<' <Exp> '>' : [6];

	%%%%

	+('*' '/') +('+' '-')
	)";

	EBNFX Tab = EBNFX::Create(Table);

	auto TableBuffer = TableWrapper::Create(Tab); // 序列化版本
	```

	一个Ebnf由三部分组成，其中以`%%%%`为分割符。

	1. 词法区

		其语法规则为：

		> `终结符` `=` `正则表达式` [`:` `标记值`] (可选)。
		
		> `$` `=` `正则表达式`

		以`$`为终结符的词法讲会被作为分隔符，被标记为分割符的词语将不会进入到后续的语法分析中。

	2. 语法区

		其语法规则为
		
		> [`非终结符`] (可选) `:=` `终结符`/`非终结符`/`常量字符串`/`标记值`/`$` ... [`:` `标记值`] (可选) `;` 

		其中`标记值`为一串数字，若跟在在一个非终结符后面，则表示防止该终结符由标记的产生式规约，若跟在产生式后，则表示标记该产生式。

		若`$`跟在一个非终结符后面，则表示防止该终结符由自身产生规约。

		当产生式左部为空时，则表示该产生式的左部为上一个产生式的左部。

		> `$` `:=` `非终结符`[`标记值`] (可选) `;`

		标记开始符号，标记值则表示当前最多支持`LR(X)`的文法。

		在产生式的右部，还支持Ebnf语法，如：
		
		> `终结符`/`非终结符`/`常量字符串`/`标记值`/`$` ... `|` `终结符`/`非终结符`/`常量字符串`/`标记值`/`$` ...

		> `[` `终结符`/`非终结符`/`常量字符串`/`标记值`/`$` ... `]`

		> `{` `终结符`/`非终结符`/`常量字符串`/`标记值`/`$` ... `}`

	3. 运算符优先级区

		该区可为空。

		一般语法规则为：

		> `+(` `常量字符串`/`终结符` ... `)`

		> `(` `常量字符串`/`终结符` ... `)+`

		在同一`()`内的所有`常量字符串`/`终结符`为相同优先级，在前面的`()`的优先级大于后面的`()`，`+()`表示左结合，`)+`表示右结合。
