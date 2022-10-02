#include "Potato/Public/PotatoSLRX.h"
#include "TestingTypes.h"
#include <iostream>

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
	Multi,
	Sub,
	Dev,
};

constexpr Symbol operator*(Terminal input) { return Symbol::AsTerminal(static_cast<std::size_t>(input)); }

enum class T : StandardT
{
	Empty = 0,
	Terminal,
	Equal,
	Rex,
	NoTerminal,
	NoProductionNoTerminal,
	StartSymbol,
	LB_Brace,
	RB_Brace,
	LM_Brace,
	RM_Brace,
	LS_Brace,
	RS_Brace,
	LeftPriority,
	RightPriority,
	Or,
	Number,
	Start,
	Colon,
	Semicolon,
	Command,
	Barrier,
	ItSelf,
};

constexpr Symbol operator*(T sym) { return Symbol::AsTerminal(static_cast<StandardT>(sym)); };

enum class NT : StandardT
{
	Statement,
	FunctionEnum,
	Expression,
	NTExpression,
	ExpressionStatement,
	ExpressionList,
	LeftOrStatement,
	RightOrStatement,
	OrStatement,
};

constexpr Symbol operator*(NT sym) { return Symbol::AsNoTerminal(static_cast<StandardT>(sym)); };

void TestingSLRX()
{

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

		throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.1" };
	}
	catch (Exception::IllegalSLRXProduction const& Wtf)
	{
		if (Wtf.Type != Exception::IllegalSLRXProduction::Category::EndlessReduce)
			throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.1" };
	}
	catch (...)
	{
		throw;
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

		throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.2" };
	}
	catch (Exception::IllegalSLRXProduction const& Wtf)
	{
		if (Wtf.Type != Exception::IllegalSLRXProduction::Category::ConfligReduce)
			throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.2" };
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
		throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.3" };
	}
	catch (...)
	{
		throw;
	}


	try {
		LRX tab(
			*Noterminal::Exp,
			{
				{*Noterminal::Exp, {*Noterminal::Exp, *Noterminal::Exp, 1}, 1},
				{*Noterminal::Exp, {*Terminal::Num}, 2},
			},
		{
		}
		, 1
		);
	}
	catch (Exception::IllegalSLRXProduction const& Wtf)
	{
		throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.4" };
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
	catch (...)
	{
		throw;
	}

	try
	{
		LRX tab(
			*Noterminal::Exp,
			{
				{*Noterminal::Exp, {*Terminal::Num}, 1},
				{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Add, *Noterminal::Exp}, 2},
				{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3},
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
			{Terminal::Num, 2}
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
			else {
				auto TE = Ele.AsTerminal();
				return Wtf[TE.Shift.TokenIndex].Value;
			}
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
	catch (Exception::UnaccableSymbol const& IL)
	{
		throw UnpassedUnit{ "TestingSLRX : Bad Output Result 1.5" };
	}

	std::wcout << LR"(TestingSLRX Pass !)" << std::endl;

}