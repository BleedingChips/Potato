import PotatoSLRX;
import std;

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

struct StringMaker : ProcessorOperator
{
	std::any operator()(Symbol Value) {
		return std::u8string(TerminalMapping[static_cast<Terminal>(Value.symbol)]);
	}

	std::any HandleReduce(SymbolInfo Symbol, ReduceProduction Production) {
		std::u8string TotalBuffer;
		TotalBuffer += u8'(';
		for (std::size_t I = 0; I < Production.Size(); ++I)
		{
			TotalBuffer += Production[I].Consume<std::u8string>();
		}
		TotalBuffer += u8')';
		return TotalBuffer;
	}
};

void TestTable(Symbol StartSymbol, std::vector<ProductionBuilder> Builder, std::vector<OpePriority> Ority, std::size_t MaxForwardDetect, std::span<Terminal const> Span, std::u8string_view TarStr, const char* Error)
{
	try {
		LRX Tab(
			StartSymbol,
			std::move(Builder),
			std::move(Ority),
			MaxForwardDetect
		);

		StringMaker Maker;

		LRXProcessor Pro;

		Pro.SetObserverTable(Tab, &Maker);

		for (std::size_t I = 0; I < Span.size(); ++I)
		{
			if(!Pro.Consume(*Span[I], {I, I + 1}, Maker(*Span[I])))
				throw Error;
		}

		if(!Pro.EndOfFile())
			throw Error;

		auto P = Pro.GetData<std::u8string>();

		if(P != TarStr)
			throw Error;

		auto Buffer = LRXBinaryTableWrapper::Create(Tab);


		Pro.SetObserverTable(LRXBinaryTableWrapper{ Buffer }, &Maker);
		Pro.Clear();

		for (std::size_t I = 0; I < Span.size(); ++I)
		{
			if (!Pro.Consume(*Span[I], { I, I + 1 }, Maker(*Span[I])))
				throw Error;
		}

		if(!Pro.EndOfFile())
			throw Error;

		auto P2 = Pro.GetData<std::u8string>();

		if (P2 != TarStr)
			throw Error;

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

	TestTable(
		*Noterminal::Exp,
		{
				{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Num}, 2},
				{*Noterminal::Exp, {}, 1},
		},
		{}, 3,
		Lists,
		u8"(((((()Num)Num)Num)Num)Num)",
		"TestingSLRX : Case 1"
	);

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