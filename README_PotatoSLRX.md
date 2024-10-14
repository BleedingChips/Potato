# PotatoSLRX

基于LR(X)的语法分析器。

```cpp
import PotatoSLRX;
using namespace Potato::SLRX;
```

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
		ProductionBuilder Builder{NoTerminlSymbol, {Symbols...}, Mask};
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