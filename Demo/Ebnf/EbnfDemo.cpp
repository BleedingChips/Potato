

#include "Potato/Include/PotatoDLr.h"
#include "Potato/Include/PotatoReg.h"
#include "Potato/Include/PotatoStrEncode.h"
#include "Potato/Include/PotatoStrFormat.h"
#include "Potato/Include/PotatoEbnf.h"
#include <iostream>

using namespace Potato;
using namespace Potato::DLr;

using Type = std::u32string_view;

using DLr::Symbol;

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

std::u32string_view EbnfCode1()
{
	return UR"(
$ := '\s+'
Num := '[1-9][0-9]*' : [1]

%%%%

$ := <Exp>;

<Exp> := Num : [1];
	:= <Exp> '+' <Exp> : [2];
	:= <Exp> '*' <Exp> : [3];
	:= <Exp> '/' <Exp> : [4];
	:= <Exp> '-' <Exp> : [5];
	:= '(' <Exp> ')' : [6];

%%%%

+('*' '/') +('+' '-')
)";
}


int main()
{

	DLr::Table tab(
		*Noterminal::Exp,
		{
			{*Noterminal::Exp, {*Terminal::Num}, 1},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Add, *Noterminal::Exp}, 2},
			{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3},
			//{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Sub, *Noterminal::Exp}, 4},
			//{*Noterminal::Exp, {*Noterminal::Exp, *Terminal::Dev, *Noterminal::Exp}, 5},
		},
		{
			{{*Terminal::Multi, *Terminal::Dev}},
			{{*Terminal::Add, *Terminal::Sub}, DLr::Associativity::Left},
		}
		);

	std::vector<Symbol> Wtf = { 
		{*Terminal::Num},
		{*Terminal::Add},
		{*Terminal::Num},
		{*Terminal::Multi},
		{*Terminal::Num}
	};

	auto Stes = DLr::ProcessSymbol(tab.Wrapper, Wtf);

	auto TableBuffer = Ebnf::TableWrapper::Create(EbnfCode1());

	auto Wrapper = Ebnf::TableWrapper{ TableBuffer };

	std::u32string_view Str = UR"(1+24+12*3-4/(1+1))";

	auto Symbols = Ebnf::ProcessSymbol(Wrapper, Str);

	auto Ite = Ebnf::ProcessStepWithOutputType<std::size_t>(Wrapper, Symbols, [=](Ebnf::StepElement Ele)->std::any{
		if (Ele.IsTerminal())
		{
			auto T = Ele.AsTerminal();
			if (T.Mask == 1)
			{
				std::size_t Result = 0;
				auto Syms = T.Index;
				StrFormat::DirectScan(Str.substr(Syms.Begin(), Syms.Count()), Result);
				return Result;
			}
		}
		else {
			auto NT = Ele.AsNoTerminal();
			switch (NT.Mask)
			{
			case 1:
				return NT[0].Consume();
			case 2:
				return NT[0].Consume<std::size_t>() + NT[2].Consume<std::size_t>();
			case 3:
				return NT[0].Consume<std::size_t>() * NT[2].Consume<std::size_t>();
			case 4:
				return NT[0].Consume<std::size_t>() / NT[2].Consume<std::size_t>();
			case 5:
				return NT[0].Consume<std::size_t>() - NT[2].Consume<std::size_t>();
			case 6:
				return NT[1].Consume();
			default:
				break;
			}
		}
		return {};
	});

	std::cout << Ite << std::endl;


	return 10;
}