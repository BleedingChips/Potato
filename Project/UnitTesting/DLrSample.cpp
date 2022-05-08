#include "Potato/Public/PotatoDLr.h"
#include "TestingTypes.h"
#include <iostream>

using namespace Potato::DLr;

enum class Noterminal
{
	Exp = 0,
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

void TestingDLr()
{

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

	Table tab(
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

	auto Stes = ProcessSymbol(tab.Wrapper, 0, [&](std::size_t Index)->Symbol {
		if (Index < Wtf.size())
			return *Wtf[Index].Ope;
		else
			return Symbol::EndOfFile();
		});

	auto Func = [&](StepElement Ele) -> std::any {
		if (Ele.IsNoTerminal())
		{
			auto NE = Ele.AsNoTerminal();
			switch (NE.Mask)
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
			return Wtf[TE.TokenIndex].Value;
		}
	};

	auto Result = ProcessStepWithOutputType<int32_t>(Stes, Func);

	if (Result != 1508)
		throw UnpassedUnit{ "TestingDLr : Bad Output Result" };

	Table tab2(
		*Noterminal::Exp,
		{
			{*Noterminal::Exp, {*Terminal::Num}, 1},
			{*Noterminal::Exp, {*Noterminal::Exp, 3, *Terminal::Add, *Noterminal::Exp, 3}, 2},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3},
			{*Noterminal::Exp, {*Noterminal::Exp, 3, *Terminal::Sub, *Noterminal::Exp, 3}, 4}
		},
		{}
	);

	auto Stes32 = ProcessSymbol(tab2.Wrapper, 0, [&](std::size_t Index)->Symbol {
		if (Index < Wtf.size())
			return *Wtf[Index].Ope;
		else
			return Symbol::EndOfFile();
		});

	auto Result2 = ProcessStepWithOutputType<int32_t>(Stes32, Func);

	if (Result2 != 1920)
		throw UnpassedUnit{ "TestingDLr : Bad Output Result2" };


	std::wcout << LR"(TestingDLr Pass !)" << std::endl;

}