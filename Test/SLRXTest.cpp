import Potato.SLRX;

using namespace Potato::SLRX;


enum class Noterminal
{
	Exp = 0,
	Exp1 = 1,
	Exp2 = 2,
};

constexpr Symbol operator*(Noterminal input) { return Symbol::AsNoTerminal(static_cast<std::size_t>(input)); }

enum class Terminal
{
	Num = 0,
	Add,
	Sub,
	Mul,
	Dev,
	LeftBracket,
	RigheBracket
};

constexpr Symbol operator*(Terminal input) { return Symbol::AsTerminal(static_cast<std::size_t>(input)); }

std::map<Terminal, std::u8string_view> TerminalMapping = {
	{Terminal::Num, std::u8string_view{u8"Num"}},
	{Terminal::Add, std::u8string_view{u8"+"}},
	{Terminal::Sub, std::u8string_view{u8"-"}},
	{Terminal::Mul, std::u8string_view{u8"*"}},
	{Terminal::Dev, std::u8string_view{u8"/"}},
	{Terminal::LeftBracket, std::u8string_view{u8"{"}},
	{Terminal::RigheBracket, std::u8string_view{u8"}"}}
};

/*
struct StringMaker
{
	struct PredictEle
	{
		ParsingStep::ReduceT Reudce;
		std::size_t LastTokenIndex;
	};

	StringMaker(const char* Error) : Error(Error) {}

	std::vector<PredictEle> PredictReduceStack;

	std::any operator()(VariantElement Element) {
		if (Element.IsTerminal())
		{
			++NextTokenIndex;
			auto Ele = Element.AsTerminal();
			return std::u8string{TerminalMapping[static_cast<Terminal>(Ele.Value.Value)]};
		}
		else if (Element.IsNoTerminal())
		{
			auto Ele = Element.AsNoTerminal();
			bool HasPredict = false;

			std::u8string Tem;

			if (!PredictReduceStack.empty() && PredictReduceStack.rbegin()->Reudce.ProductionIndex == Ele.Reduce.ProductionIndex)
			{
				auto Last = *PredictReduceStack.rbegin();
				PredictReduceStack.pop_back();
				if (Ele.Datas.size() == 0 && Last.LastTokenIndex == NextTokenIndex || Ele.Datas.size() != 0 && Last.LastTokenIndex == Ele.Datas[0].Mate.TokenIndex.Begin())
				{
					Tem += u8"&(";
				}
				else {
					throw Error ;
				}
			}
			else {
				Tem += u8"(";
			}
			for (auto& Ite : Ele.Datas)
			{
				Tem += Ite.Consume<std::u8string>();
			}
			Tem += u8")";
			return Tem;
		}
		else if (Element.IsPredict())
		{
			auto Ele = Element.AsPredict();
			PredictReduceStack.push_back({Ele.Reduce, NextTokenIndex });
			return {};
		}
		else {
			return {};
		}
	}

	std::size_t NextTokenIndex = 0;

	const char* Error;
};
*/

void TestTable(Symbol StartSymbol, std::vector<ProductionBuilder> Builder, std::vector<OpePriority> Ority, std::size_t MaxForwardDetect, std::span<Terminal const> Span, std::u8string_view TarStr, const char* Error)
{
	try {
		LRX Tab(
			StartSymbol,
			std::move(Builder),
			std::move(Ority),
			MaxForwardDetect
		);

		LRXProcessor<std::size_t>

		/*
		SymbolProcessor Pro(Tab);

		auto IteSpan = Span;
		while (!IteSpan.empty())
		{
			auto Top = *IteSpan.begin();
			if (!Pro.Consume(*Top, Span.size() - IteSpan.size()))
				throw Error;
			IteSpan = IteSpan.subspan(1);
		}
		if (!Pro.EndOfFile())
			throw Error;

		StringMaker Maker(Error);

		auto Re = ProcessParsingStepWithOutputType<std::u8string>(Pro.GetSteps(), Maker);

		if (Re != TarStr)
			throw Error;

		auto Buffer = LRXBinaryTableWrapper::Create(Tab);

		SymbolProcessor Pro3(LRXBinaryTableWrapper{ Buffer });

		IteSpan = Span;
		while (!IteSpan.empty())
		{
			auto Top = *IteSpan.begin();
			if (!Pro3.Consume(*Top, Span.size() - IteSpan.size()))
				throw Error;
			IteSpan = IteSpan.subspan(1);
		}
		if (!Pro3.EndOfFile())
			throw Error;

		StringMaker Maker2(Error);

		auto Re2 = ProcessParsingStepWithOutputType<std::u8string>(Pro3.GetSteps(), Maker2);

		if (Re2 != TarStr)
			throw Error;
			*/
	}
	catch (Exception::Interface const&)
	{
		throw Error;
	}
}


void TestingSLRX()
{ 

	std::vector<Terminal> Lists = { Terminal::Num, Terminal::Num, Terminal::Num, Terminal::Num, Terminal::Num };
	std::vector<Terminal> Lists2 = { Terminal::Num, Terminal::Add, Terminal::Num, Terminal::Mul, Terminal::Num, Terminal::Add, Terminal::Num };
	std::vector<Terminal> Lists3 = { Terminal::Num, Terminal::Num, Terminal::Num, Terminal::Num, Terminal::Num, Terminal::Num };

	/*
	TestTable(
		*Noterminal::Exp,
		{
				{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Num}, 2, true},
				{*Noterminal::Exp, {}, 1, true},
		},
		{}, 3,
		Lists,
		u8"&(&(&(&(&(&()Num)Num)Num)Num)Num)",
		"TestingSLRX : Case 1"
	);
	*/

	TestTable(
		*Noterminal::Exp,
		{
				{*Noterminal::Exp, {*Noterminal::Exp, 2, *Noterminal::Exp}, 2},
				{*Noterminal::Exp, {*Terminal::Num}, 1},
		},
		{}, 3,
		Lists,
		u8"((Num)((Num)((Num)((Num)(Num)))))",
		"TestingSLRX : Case 2"
	);

	

	TestTable(
		*Noterminal::Exp,
		{
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Add, *Noterminal::Exp}, 3},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Mul, *Noterminal::Exp}, 4},
			{*Noterminal::Exp, {*Terminal::Num}, 1},
		},
		{
			{{*Terminal::Mul}, OpePriority::Associativity::Left},
			{{*Terminal::Add}, OpePriority::Associativity::Right},
		}, 3,
		Lists2,
		u8"((Num)+(((Num)*(Num))+(Num)))",
		"TestingSLRX : Case 3"
	);

	TestTable(
		*Noterminal::Exp,
		{
			{*Noterminal::Exp, {*Terminal::Num, *Terminal::Num, *Terminal::Num, *Terminal::Num, *Terminal::Num, *Terminal::Add}, 3},
			{*Noterminal::Exp, {*Noterminal::Exp1, *Terminal::Num, *Terminal::Num, *Terminal::Num, *Terminal::Num, *Terminal::Num}, 3},
			{*Noterminal::Exp1, {*Terminal::Num}, 4},
		},
		{
		}, 5,
		Lists3,
		u8"((Num)NumNumNumNumNum)",
		"TestingSLRX : Case 3"
		);


	/*
	try {
		LRX Tab(
			*Noterminal::Exp,
			{
				{*Noterminal::Exp, {*Noterminal::Exp, *Noterminal::Exp}, 2},
				{*Noterminal::Exp, {}},
			},
		{
		}
		);

		throw UnpassedUnit{ "TestingSLRX : Case 1" };
	}
	catch (Exception::IllegalSLRXProduction const& Wtf)
	{
		if (Wtf.Type != Exception::IllegalSLRXProduction::Category::EndlessReduce)
			throw UnpassedUnit{ "TestingSLRX : Case 1" };
	}



	try {
		LRX Tab(
			*Noterminal::Exp,
			{
				{*Noterminal::Exp, {*Noterminal::Exp, *Noterminal::Exp}, 2},
				{*Noterminal::Exp, {*Terminal::Num}},
			},
		{
		}
		);

		throw UnpassedUnit{ "TestingSLRX : Case 2" };
	}
	catch (Exception::IllegalSLRXProduction const& Wtf)
	{
		if (Wtf.Type != Exception::IllegalSLRXProduction::Category::ConfligReduce)
			throw UnpassedUnit{ "TestingSLRX : Case 2" };
	}
	catch (...)
	{
		throw;
	}


	try {
		LRX tab(
			*Noterminal::Exp,
			{
				{*Noterminal::Exp, {*Noterminal::Exp1, *Terminal::Num, *Terminal::Num, *Terminal::Num, *Terminal::Add}, 1},
				{*Noterminal::Exp, {*Noterminal::Exp2, *Terminal::Num, *Terminal::Num, *Terminal::Num, *Terminal::Num}, 2},
				{*Noterminal::Exp1, {*Terminal::Num}, 3},
				{*Noterminal::Exp2, {*Terminal::Num}, 4},
			},
		{
		},
		4
		);
	}
	catch (Exception::IllegalSLRXProduction const&)
	{
		throw UnpassedUnit{ "TestingSLRX : Case 3" };
	}


	try {
		LRX tab(
			*Noterminal::Exp,
			{
				{*Noterminal::Exp, {*Noterminal::Exp, *Noterminal::Exp, 1}, 1, true},
				{*Noterminal::Exp, {*Terminal::Num}, 2},
			},
		{
		}
		, 1
		);
	}
	catch (Exception::IllegalSLRXProduction const& Wtf)
	{
		throw UnpassedUnit{ "TestingSLRX : Case 4" };
	}
	catch (...)
	{
		throw;
	}
	

	try {
		LRX tab(
			*Noterminal::Exp,
			{
				{*Noterminal::Exp, {*Noterminal::Exp, *Noterminal::Exp, ItSelf{}}, 1},
				{*Noterminal::Exp, {*Terminal::Num}, 2},
			},
		{
		}
		, 1
		);

		SymbolProcessor Pro(tab);

		std::vector<Symbol> Symbols = {*Terminal::Num, *Terminal::Num , *Terminal::Num , *Terminal::Num };

		std::size_t Count = 0;
		for (auto Ite : Symbols)
		{
			auto Re = Pro.Consume(Ite, Count);
			if(!Re)
				throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.4" };
			++Count;
		}
		auto K = Pro.EndOfFile();
		if(!K)
			throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.4" };
	}
	catch (Exception::IllegalSLRXProduction const& Wtf)
	{
		throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.4" };
	}

	/*
	try
	{
		LRX tab(
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

		struct SymbolTuple
		{
			Terminal Ope;
			int32_t Value;
		};

		std::vector<SymbolTuple> Wtf = {
			{Terminal::Num, 10},
			{Terminal::Add, 20},
			{Terminal::Num, 30},
			{Terminal::Multi, 40},
			{Terminal::Num, 50},
			{Terminal::Sub, 20},
			{Terminal::Num, 2},
			{Terminal::Add, 20},
			{Terminal::Num, 0}
		};

		SymbolProcessor Pro(tab);

		std::size_t O = 0;
		for (auto& Ite : Wtf)
		{
			auto P = Pro.Consume(*Ite.Ope, O++);
			if(!P)
				throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.5" };
			volatile int i = 0;
		}

		auto End = Pro.EndOfFile();

		if(!End)
			throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.5" };

		auto Func = [&](VariantElement Ele) -> std::any {
			if (Ele.IsNoTerminal())
			{
				auto NE = Ele.AsNoTerminal();
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
				auto TE = Ele.AsTerminal();
				return Wtf[TE.Shift.TokenIndex].Value;
			}
			return {};
		};

		auto Result = ProcessParsingStepWithOutputType<int32_t>(Pro.GetSteps(), Func);

		if (Result != 1508)
			throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.5" };

		{
			auto Buffer = TableWrapper::Create(tab);

			SymbolProcessor Pro2(TableWrapper{ Buffer });

			std::size_t O = 0;
			for (auto& Ite : Wtf)
			{
				auto P = Pro2.Consume(*Ite.Ope, O++);
				volatile int i = 0;
			}
			Pro2.EndOfFile();

			auto Result = ProcessParsingStepWithOutputType<int32_t>(Pro2.GetSteps(), Func);

			if(Result != 1508)
				throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.5" };
		}
		
	}
	catch (Exception::IllegalSLRXProduction const&)
	{
		throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.5" };
	}
	*/

	std::cout << "TestingSLRX Pass !" << std::endl;

}

int main()
{
	try
	{
		TestingSLRX();
	}
	catch (char const* Error)
	{
		std::cout << Error << std::endl;
		return -1;
	}
	return 0;
}